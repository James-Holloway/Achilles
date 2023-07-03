#pragma once

static LRESULT CALLBACK AchillesWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Achilles
{
protected:
	static const uint8_t numFrames = 3; // Number of swap chain back buffers
	// Window
	HWND hWnd = NULL; // window
	RECT windowRect = RECT(); // stores non-fullscreen window state when returning from FS
	bool useWarp = false; // Use WARP adapter - https://learn.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp
	bool fullscreen = false;
	bool vSync = true; // VSync
	bool tearingSupported = false;

	// Window width and height
	uint32_t clientWidth = 1280;
	uint32_t clientHeight = 720;

	// DirectX 12 Objects
	ComPtr<ID3D12Device2> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> backBuffers[numFrames];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandAllocator> commandAllocators[numFrames];
	ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap; // Render Target view Descriptor Heap0

	UINT RTVDescriptorSize = 0;
	UINT currentBackBufferIndex = 0;

	// Synchronization objects
	ComPtr<ID3D12Fence> fence;
	uint64_t fenceValue = 0;
	uint64_t frameFenceValues[numFrames] = {};
	HANDLE fenceEvent = NULL;

	bool isInitialized = false; // Stores init state

	// Update variables
	uint64_t frameCounter = 0;
	double elapsedSeconds = 0.0;
	std::chrono::high_resolution_clock clock;
	std::chrono::steady_clock::time_point prevClock;

public:
	// Public statics for internal use
	inline static std::map<HWND, Achilles*> instanceMapping = std::map<HWND, Achilles*>();

public:
	// Public variables
	FLOAT clearColor[4] = { 0.4f, 0.58f, 0.93, 1.0f}; // Cornflower Blue

protected:
	// Protected variables
	std::wstring name = L"Achilles";

public:
	// Constructor and destructor functions
	Achilles();
	Achilles(std::wstring name);

protected:
	// Setup functions
	void ParseCommandLineArguments();
	void EnableDebugLayer();
	static bool CheckTearingSupport();

	void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName); // Register the window class using WndProc
	HWND AchillesCreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height);

	ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device);

	// Event functions
	HANDLE CreateEventHandle();
	uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
	void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent);

public:
	// Public functions used internally
	LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
	// Achilles functions
	void Update(); // Optional (call Run otherwise)
	void Render(); // Optional (call Run otherwise)
	void Resize(uint32_t width, uint32_t height); // Optional (call Run otherwise)
	void SetFullscreen(bool fs);
	void Run();

	void Destroy(); // Optional (call Run otherwise)
};