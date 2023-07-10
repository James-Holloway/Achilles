#include "Achilles.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

static LRESULT CALLBACK AchillesWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (Achilles::instanceMapping.contains(hwnd))
	{
		Achilles* achilles = Achilles::instanceMapping[hwnd];
		return achilles->WndProc(hwnd, uMsg, wParam, lParam);
	}
	else
	{
		switch (uMsg) {
		case WM_CREATE:
			return 0;
		case WM_PAINT:
			return 0;
		case WM_SIZE:
			return 0;
		case WM_DESTROY:
			return 0;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	return 0;
}

Achilles::Achilles()
{
	prevUpdateClock = clock.now();
	prevRenderClock = clock.now();
}

Achilles::Achilles(std::wstring _name)
{
	prevUpdateClock = clock.now();
	prevRenderClock = clock.now();
	name = _name;
}

Achilles::~Achilles()
{
	assert(!hWnd && "Use Achilles::Destroy before destruction!");
}

void Achilles::ParseCommandLineArguments()
{
	int argc;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	for (size_t i = 0; i < argc; ++i)
	{
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
		{
			clientWidth = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
		{
			clientHeight = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
		{
			useWarp = true;
		}
	}

	// Free memory allocated by CommandLineToArgvW
	::LocalFree(argv);
}

void Achilles::EnableDebugLayer()
{
#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related so all possible errors generated while creating DX12 objects are caught by the debug layer
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

bool Achilles::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	// Rather than create the DXGI 1.5 factory interface directly, we create the DXGI 1.4 interface and query for the 1.5 interface.
	// This is to enable the graphics debugging tools which will not support the 1.5 factory interface until a future update.
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}

void Achilles::RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
	// Register a window class for creating our render window with
	WNDCLASSEXW windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &AchillesWndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInst;
	windowClass.hIcon = ::LoadIconW(hInst, NULL);
	windowClass.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = ::LoadIconW(hInst, NULL);

	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassexa
	static ATOM atom = ::RegisterClassExW(&windowClass);
	assert(atom > 0);
}

HWND Achilles::AchillesCreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowExW(
		NULL, // allow drag and drop files to load fbxs: WS_EX_ACCEPTFILES
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		hInst,
		nullptr
	);

	assert(hWnd && "Failed to create window");

	instanceMapping[hWnd] = this;

	return hWnd;
}

ComPtr<IDXGIAdapter4> Achilles::GetAdapter(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		// Create the warp adapter
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}

ComPtr<ID3D12Device2> Achilles::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		// pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, TRUE);
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

ComPtr<IDXGISwapChain4> Achilles::CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}

void Achilles::UpdateRenderTargetViews()
{
	for (int i = 0; i < BufferCount; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		ResourceStateTracker::AddGlobalResourceState(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

		backBuffers[i] = std::make_shared<Texture>(backBuffer, nullptr, TextureUsage::RenderTarget);

		wchar_t buff[32];
		swprintf_s(buff, L"Back Buffer %i/%i", i + 1, BufferCount);
		backBuffers[i]->SetName(buff);
	}
}

void Achilles::UpdateDepthStencilView()
{
	DXGI_SAMPLE_DESC sampleDesc = { 1,0 };

	CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, clientWidth, clientHeight, 1, 1, sampleDesc.Count, sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	std::shared_ptr<Texture> depthBuffer = std::make_shared<Texture>(depthDesc, &depthClearValue, TextureUsage::Depth, L"DepthStencil RT");
	depthBuffer->CreateViews();
	ResourceStateTracker::AddGlobalResourceState(depthBuffer->GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COMMON);

	renderTarget->AttachTexture(AttachmentPoint::DepthStencil, depthBuffer);
}

std::shared_ptr<Texture> Achilles::CreateRenderTargetTexture(std::wstring name)
{
	DXGI_SAMPLE_DESC sampleDesc = { 1,0 };

	CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, clientWidth, clientHeight, 1, 1, sampleDesc.Count, sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	std::shared_ptr<Texture> rtTexture = std::make_shared<Texture>(depthDesc, &depthClearValue, TextureUsage::Albedo, name);
	rtTexture->CreateViews();
	ResourceStateTracker::AddGlobalResourceState(rtTexture->GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COMMON);

	return rtTexture;
}

void Achilles::UpdateMainRenderTarget()
{
	renderTarget->Reset();
	std::shared_ptr<Texture> rtTexture = Achilles::CreateRenderTargetTexture(L"Main Render Target");
	renderTarget->AttachTexture(AttachmentPoint::Color0, rtTexture);
	renderTarget->Resize(clientWidth, clientHeight);
	UpdateDepthStencilView();
}

ComPtr<ID3D12CommandAllocator> Achilles::CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

// Get functions
std::shared_ptr<CommandQueue> Achilles::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	return std::make_shared<CommandQueue>(type);
}

std::shared_ptr<RenderTarget> Achilles::GetCurrentRenderTarget() const
{
	return renderTarget;
}

std::shared_ptr<RenderTarget> Achilles::GetSwapChainRenderTarget() const
{
	swapChainRenderTarget->AttachTexture(AttachmentPoint::Color0, backBuffers[currentBackBufferIndex]);
	return swapChainRenderTarget;
}

// Stall CPU while we signal and wait
void Achilles::Flush()
{
	directCommandQueue->Flush();
	computeCommandQueue->Flush();
	copyCommandQueue->Flush();
}

LRESULT Achilles::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (isInitialized)
	{
		LRESULT imGuiReturn = achillesImGui->WndProcHandler(hwnd, message, wParam, lParam);
		if (imGuiReturn != TRUE)
		{
			switch (message)
			{
			case WM_PAINT:
				Update();
				Render();
				break;
			case WM_SIZE:
			{
				RECT clientRect = {};
				::GetClientRect(hWnd, &clientRect);

				int width = clientRect.right - clientRect.left;
				int height = clientRect.bottom - clientRect.top;

				Resize(width, height);
			}
			// The default window procedure will play a system notification sound when pressing the Alt+Enter keyboard combination if this message is not handled.
			case WM_SYSCHAR:
				break;
			case WM_DESTROY:
				::PostQuitMessage(0);
				break;

				// DirectXTK12 Keyboard.h processing
			case WM_ACTIVATE:
			case WM_ACTIVATEAPP:
				Keyboard::ProcessMessage(message, wParam, lParam);
				Mouse::ProcessMessage(message, wParam, lParam);
				break;
			case WM_SYSKEYDOWN:
				if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
				{
					SetFullscreen(!fullscreen);
				}
				Keyboard::ProcessMessage(message, wParam, lParam);
				break;
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYUP:
				Keyboard::ProcessMessage(message, wParam, lParam);
				break;
			case WM_MENUCHAR:
				// A menu is active and the user presses a key that does not correspond to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
				return MAKELRESULT(0, MNC_CLOSE);

				// DirectXTK12 Mouse.h processing
			case WM_INPUT:
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEWHEEL:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP:
			case WM_MOUSEHOVER:
				Mouse::ProcessMessage(message, wParam, lParam);
				break;
			case WM_MOUSEACTIVATE:
				// When you click to activate the window, we want Mouse to ignore that event.
				return MA_ACTIVATEANDEAT;
			default:
				return ::DefWindowProcW(hwnd, message, wParam, lParam);
			}
		}
		else // if ImGui returned TRUE
		{
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}
	else // not initalized
	{
		return ::DefWindowProcW(hwnd, message, wParam, lParam);
	}
	return 0;
}

// Protected Achilles functions
void Achilles::HandleKeyboard()
{
	auto keyboardState = keyboard->GetState();
	keyboardTracker.Update(keyboardState);

	if (keyboardTracker.pressed.Escape)
	{
		PostQuitMessage(0);
	}

	if (keyboardTracker.pressed.V)
	{
		vSync = !vSync;
	}

	if (keyboardTracker.pressed.F11)
	{
		SetFullscreen(!fullscreen);
	}
}

void Achilles::HandleMouse(int& mouseX, int& mouseY, int& scroll, Mouse::State& state)
{
	state = mouse->GetState();
	mouseTracker.Update(state);
	mouseX = state.x;
	mouseY = state.y;
	scroll = state.scrollWheelValue;
}

void Achilles::Present(std::shared_ptr<CommandQueue> commandQueue, std::shared_ptr<CommandList> commandList)
{
	std::shared_ptr<Texture> backBuffer = backBuffers[currentBackBufferIndex];

	std::shared_ptr<RenderTarget> currentRT = GetCurrentRenderTarget();
	std::shared_ptr<Texture> texture = currentRT->GetTexture(AttachmentPoint::Color0);

	if (texture && texture->IsValid())
	{
		if (texture->GetD3D12ResourceDesc().SampleDesc.Count > 1)
		{
			commandList->ResolveSubresource(*backBuffer, *texture);
		}
		else
		{
			commandList->CopyResource(*backBuffer, *texture);
		}
	}

	commandList->TransitionBarrier(*backBuffer, D3D12_RESOURCE_STATE_PRESENT);

	fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

	UINT syncInterval = vSync ? 1 : 0;
	UINT presentFlags = tearingSupported && !vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;

	ThrowIfFailed(swapChain->Present(syncInterval, presentFlags));
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	fenceValues[currentBackBufferIndex] = commandQueue->Signal();
	frameValues[currentBackBufferIndex] = Application::GetGlobalFrameCounter();

	// Stall CPU thread until GPU has executed
	commandQueue->WaitForFenceValue(fenceValues[currentBackBufferIndex]);

	Application::ReleaseStaleDescriptors(frameValues[currentBackBufferIndex]);
}

// Achilles functions
void Achilles::Update()
{
	std::chrono::steady_clock::time_point currClock = clock.now();
	std::chrono::duration<long long, std::nano> deltaTime = currClock - prevUpdateClock;
	prevUpdateClock = currClock;

	float dt = deltaTime.count() * 1e-9f;
	totalElapsedSeconds += dt;
	elapsedSeconds += dt;

	// Print FPS every half second
	if (elapsedSeconds > 0.1)
	{
		double fps = frameCounter / elapsedSeconds;
		// OutputDebugStringWFormatted(L"FPS: %.1f\n", fps);

		frameCounter = 0;
		elapsedSeconds = 0.0;
		lastFPS = fps;

		historicalFrameTimes.push_front(dt);
		if (historicalFrameTimes.size() > 150 + 1) // past 15 seconds (pushed every 0.1 second)
		{
			historicalFrameTimes.pop_back();
		}
	}

	HandleKeyboard();

	int mouseX = 0, mouseY = 0, scroll = 0;
	Mouse::State mouseState;
	HandleMouse(mouseX, mouseY, scroll, mouseState);
	MouseData mouseData{};
	mouseData.mouseX = mouseX;
	mouseData.mouseY = mouseY;
	mouseData.scroll = scroll;
	mouseData.mouseXDelta = mouseData.mouseX - prevMouseData.mouseX;
	mouseData.mouseYDelta = mouseData.mouseY - prevMouseData.mouseY;
	mouseData.scrollDelta = mouseData.scroll - prevMouseData.scroll;
	prevMouseData = mouseData;

	if (!ImGui::GetIO().WantCaptureKeyboard)
		OnKeyboard(keyboardTracker, keyboard->GetState(), dt);
	if (!ImGui::GetIO().WantCaptureMouse)
		OnMouse(mouseTracker, mouseData, mouseState, dt);

	OnUpdate(dt);
}

void Achilles::Render()
{
	frameCounter++;
	std::chrono::steady_clock::time_point currClock = clock.now();
	std::chrono::duration<long long, std::nano> deltaTime = currClock - prevRenderClock;
	prevRenderClock = currClock;

	ComPtr<ID3D12CommandAllocator> commandAllocator = commandAllocators[currentBackBufferIndex];
	commandAllocator->Reset();
	std::shared_ptr<CommandList> directCommandList = directCommandQueue->GetCommandList();

	std::shared_ptr<RenderTarget> rt = GetCurrentRenderTarget();
	std::shared_ptr<Texture> rtTexture = rt->GetTexture(AttachmentPoint::Color0);
	// Clear render target
	{
		// Transition into a state from which we can clear the RT
		directCommandList->TransitionBarrier(*rtTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Clear the render target
		if (rtTexture)
			directCommandList->ClearTexture(*rtTexture, clearColor);

		// Clear the render depth
		std::shared_ptr<Texture> depthTexture = rt->GetTexture(AttachmentPoint::DepthStencil);
		if (depthTexture)
			directCommandList->ClearDepthStencilTexture(*depthTexture, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
	}

	directCommandList->SetRenderTarget(*rt);

	achillesImGui->NewFrame();

	float dt = deltaTime.count() * 1e-9f;

	DrawActiveScenes();

	OnRender(dt);

	DrawQueuedEvents(directCommandList);

	OnPostRender(dt);


	directCommandQueue->ExecuteCommandList(directCommandList);
	std::shared_ptr<CommandList> presentCommandList = directCommandQueue->GetCommandList();

	achillesImGui->Render(presentCommandList, *GetCurrentRenderTarget());

	Present(directCommandQueue, presentCommandList);
	Flush();

	totalFrameCount++;
	Application::IncrementGlobalFrameCounter();
}

void Achilles::Resize(uint32_t width, uint32_t height)
{
	if (clientWidth != width || clientHeight != height)
	{
		// Don't allow 0 size swap chain back buffers.
		clientWidth = std::max(1u, width);
		clientHeight = std::max(1u, height);

		// Flush the GPU queue to make sure the swap chain's back buffers are not being referenced by an in-flight command list
		Flush();

		renderTarget->Reset();
		for (int i = 0; i < BufferCount; ++i)
		{
			ResourceStateTracker::RemoveGlobalResourceState(backBuffers[i]->GetD3D12Resource().Get());
			// Any references to the back buffers must be released before the swap chain can be resized
			backBuffers[i]->Reset();
			backBuffers[i].reset();
			fenceValues[i] = fenceValues[currentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(swapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(swapChain->ResizeBuffers(BufferCount, clientWidth, clientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

		UpdateMainRenderTarget();
		UpdateRenderTargetViews();

		Flush();

		OnResize(clientWidth, clientHeight);
	}
}

void Achilles::SetFullscreen(bool fs)
{
	if (fullscreen != fs)
	{
		fullscreen = fs;

		if (fullscreen) // Switching to fullscreen.
		{
			// Store the current window dimensions so they can be restored when switching out of fullscreen state.
			::GetWindowRect(hWnd, &windowRect);
			// Set the window style to a borderless window so the client area fills the entire screen.
			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(hWnd, GWL_STYLE, windowStyle);

			// Query the name of the nearest display device for the window
			// This is required to set the fullscreen dimensions of the window when using a multi-monitor setup.
			HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);

			::SetWindowPos(hWnd, HWND_TOP,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(hWnd, SW_MAXIMIZE);
		}
		else
		{
			// Restore all the window decorators.
			::SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(hWnd, HWND_NOTOPMOST,
				windowRect.left,
				windowRect.top,
				windowRect.right - windowRect.left,
				windowRect.bottom - windowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(hWnd, SW_NORMAL);
		}
	}
}

void Achilles::Initialize()
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window to achieve 100% scaling while still allowing non-client window content to be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	ParseCommandLineArguments();
	EnableDebugLayer();

	// Initialize COM, used for Texture loading using CommandList::LoadTextureFromFile
	ThrowIfFailed(CoInitializeEx(NULL, COINIT_MULTITHREADED));

	std::wstring windowClassName = (L"AchillesWindowClass" + name);

	tearingSupported = CheckTearingSupport();

	HINSTANCE hInstance = HINST_THISCOMPONENT;

	RegisterWindowClass(hInstance, windowClassName.c_str());
	hWnd = AchillesCreateWindow(windowClassName.c_str(), hInstance, name.c_str(), clientWidth, clientHeight);

	// Initialize the global window rect variable.
	::GetWindowRect(hWnd, &windowRect);

	// Initialize the D3D12 objects
	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(useWarp);

	device = CreateDevice(dxgiAdapter4);

	Application::SetD3D12Device(device);

	directCommandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	computeCommandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	copyCommandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

	Application::SetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, directCommandQueue);
	Application::SetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE, computeCommandQueue);
	Application::SetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY, copyCommandQueue);

	Application::CreateDescriptorAllocators();

	swapChain = CreateSwapChain(hWnd, directCommandQueue->GetD3D12CommandQueue(), clientWidth, clientHeight, BufferCount);

	renderTarget = std::make_shared<RenderTarget>();
	UpdateMainRenderTarget();
	swapChainRenderTarget = std::make_shared<RenderTarget>();

	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	UpdateRenderTargetViews();

	for (int i = 0; i < BufferCount; ++i)
	{
		commandAllocators[i] = CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	Flush();

	achillesImGui = std::make_shared<AchillesImGui>(device, hWnd, *GetSwapChainRenderTarget());

	// Init Achilles objects
	keyboard = std::make_unique<Keyboard>();
	mouse = std::make_unique<Mouse>();
	mouse->SetWindow(hWnd);

	mainScene = std::make_shared<Scene>(L"Main Scene");
	AddScene(mainScene);

	isInitialized = true;

	LoadContent();

	mainScene->SetActive(true);
}

void Achilles::Run()
{
	if (!isInitialized || hWnd == NULL)
		throw std::exception();

	// Show window
	::ShowWindow(hWnd, SW_SHOW);

	// Peek, translate and dispatch messages
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	// Make sure the command queue has finished all commands before closing.
	Flush();

	Destroy();
}

void Achilles::Destroy()
{
	EmptyDrawQueue();
	UnloadContent();

	for (int i = 0; i < BufferCount; ++i)
	{
		backBuffers[i]->Reset();
		backBuffers[i].reset();
	}

	// swapChain.Reset();
	device.Reset();

	// Reset application so we're not holding onto references that we shouldn't be
	Application::SetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, nullptr);
	Application::SetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE, nullptr);
	Application::SetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY, nullptr);
	Application::ResetD3D12Device();

	// Release the command queues so they for-sure happen before the swapchain gets destructed
	directCommandQueue.reset();
	computeCommandQueue.reset();
	copyCommandQueue.reset();

	// Remove instance from instance mappings
	instanceMapping.erase(hWnd);
	DestroyWindow(hWnd);
	hWnd = nullptr;
}

// Achilles functions for creating things
std::shared_ptr<Texture> Achilles::CreateTexture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
{
	return std::make_shared<Texture>(resourceDesc, clearValue);
}

std::shared_ptr<Texture> Achilles::CreateTexture(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue)
{
	return std::make_shared<Texture>(resource, clearValue);
}

// Scene functions
std::shared_ptr<Scene> Achilles::GetMainScene()
{
	return mainScene;
}

void Achilles::DrawActiveScenes()
{
	for (std::shared_ptr<Scene> scene : scenes)
	{
		if (scene->IsActive())
			QueueSceneDraw(scene);
	}
}

void Achilles::AddScene(std::shared_ptr<Scene> scene)
{
	scenes.emplace(scene);
}

void Achilles::RemoveScene(std::shared_ptr<Scene> scene)
{
	scenes.erase(scene);
}

// Achilles drawing functions
void Achilles::QueueMeshDraw(std::shared_ptr<Mesh> mesh)
{
	if (mesh == nullptr)
		return;
	DrawEvent de{};
	de.camera = Camera::mainCamera;
	de.mesh = mesh;
	de.eventType = DrawEventType::DrawIndexed;
	drawEventQueue.push(de);
}

void Achilles::QueueSceneDraw(std::shared_ptr<Scene> scene)
{
	if (scene == nullptr)
		return;

	std::vector<std::shared_ptr<Object>> flattenedScene;
	scene->GetObjectTree()->FlattenActive(flattenedScene);
	for (std::shared_ptr<Object> object : flattenedScene)
	{
		QueueMeshDraw(object->GetMesh());
	}
}

void Achilles::DrawMeshIndexed(std::shared_ptr<CommandList> commandList, std::shared_ptr<Mesh> mesh, std::shared_ptr<Camera> camera)
{
	if (mesh == nullptr)
		throw std::exception("Rendered mesh was not available");
	if (camera.use_count() <= 0)
		throw std::exception("Rendered camera was not available");

	Material material = mesh->material;
	std::shared_ptr<Shader> shader = material.shader;
	if (shader->renderCallback == nullptr)
		throw std::exception("Shader did not have a rendercallback");

	std::shared_ptr<RenderTarget> rt = GetCurrentRenderTarget();

	commandList->SetPipelineState(shader->pipelineState);
	commandList->SetGraphicsRootSignature(*shader->rootSignature);

	commandList->SetPrimitiveTopology(mesh->topology);
	commandList->SetVertexBuffer(0, *mesh->vertexBuffer);
	commandList->SetIndexBuffer(*mesh->indexBuffer);

	commandList->SetViewport(camera->viewport);
	commandList->SetScissorRect(camera->scissorRect);

	commandList->SetRenderTarget(*rt);

	shader->renderCallback(commandList, mesh, material, camera);

	commandList->DrawIndexed((uint32_t)mesh->indexBuffer->GetNumIndicies(), 1, 0, 0, 0);
}

void Achilles::DrawQueuedEvents(std::shared_ptr<CommandList> commandList)
{
	while (!drawEventQueue.empty())
	{
		DrawEvent de = drawEventQueue.front();
		drawEventQueue.pop();
		switch (de.eventType)
		{
		case DrawEventType::Ignore:
			break;
		case DrawEventType::DrawIndexed:
			DrawMeshIndexed(commandList, de.mesh, de.camera);
			break;
		}
	}
}

void Achilles::EmptyDrawQueue()
{
	std::queue<DrawEvent>().swap(drawEventQueue);
}