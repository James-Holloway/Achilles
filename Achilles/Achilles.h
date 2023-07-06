#pragma once
#include "Common.h"
#include "CommandQueue.h"
#include "Resources.h"
#include "Camera.h"
#include "Mesh.h"
#include "DrawEvent.h"
#include "MouseData.h"

using Microsoft::WRL::ComPtr;

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
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> backBuffers[numFrames];
	std::shared_ptr<CommandQueue> directCommandQueue;
	std::shared_ptr<CommandQueue> computeCommandQueue;
	std::shared_ptr<CommandQueue> copyCommandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocators[numFrames];
	ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap; // Render Target View Descriptor Heap
	ComPtr<ID3D12Resource> depthBuffer;
	ComPtr<ID3D12DescriptorHeap> DSVDescriptorHeap; // Depth Stencil View Descriptor Heap

	UINT RTVDescriptorSize = 0;
	UINT currentBackBufferIndex = 0;

	// Synchronization objects
	uint64_t frameFenceValues[numFrames] = {};

	bool isInitialized = false; // Stores init state

private:
	// Update variables
	uint64_t frameCounter = 0;
	double elapsedSeconds = 0.0;
	std::chrono::high_resolution_clock clock;
	std::chrono::steady_clock::time_point prevUpdateClock;
	std::chrono::steady_clock::time_point prevRenderClock;
protected:
	// Less private update variables
	uint64_t totalFrameCount = 0;
	double totalElapsedSeconds = 0;

public:
	// Public statics for internal use
	inline static std::map<HWND, Achilles*> instanceMapping = std::map<HWND, Achilles*>();

public:
	// Public variables
	FLOAT clearColor[4] = { 0.4f, 0.58f, 0.93f, 1.0f }; // Cornflower Blue

protected:
	// Protected variables for internal use
	std::wstring name = L"Achilles";
	// Keyboard tracking
	std::unique_ptr<DirectX::Keyboard> keyboard;
	DirectX::Keyboard::KeyboardStateTracker keyboardTracker;
	std::unique_ptr<DirectX::Mouse> mouse;
	DirectX::Mouse::ButtonStateTracker mouseTracker;
	MouseData prevMouseData{};

	// Achilles drawing internals
	std::queue<DrawEvent> drawEventQueue;

public:
	// Constructor and destructor functions
	Achilles();
	Achilles(std::wstring name);
	~Achilles();

protected:
	// Setup functions
	void ParseCommandLineArguments();
	void EnableDebugLayer();
	static bool CheckTearingSupport();

	void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName); // Register the window class using WndProc
	HWND AchillesCreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height);

	ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
	ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);

	// Clear functions
	void ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);
	void ClearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	// Get functions
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

	// Resource, command queue and command list functions
	void Flush();
	void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	void ResizeDepthBuffer(int width, int height);

public:
	// Public functions used internally
	LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
	// Protected Achilles functions
	void HandleKeyboard();
	void HandleMouse(int& mouseX, int& mouseY, int& scroll, DirectX::Mouse::State& state);
	void Present(std::shared_ptr<CommandQueue> commandQueue, ComPtr<ID3D12GraphicsCommandList2> commandList);

public:
	// Achilles functions to run things
	void Update(); // Optional (call Run otherwise)
	void Render(); // Optional (call Run otherwise)
	void Resize(uint32_t width, uint32_t height); // Optional (call Run otherwise)
	void SetFullscreen(bool fs);
	void Initialize();
	void Run();
	void Destroy(); // Optional (call Run otherwise)

public:
	// Achilles callbacks
	virtual void OnUpdate(float deltaTime) {}; // Post internal Update
	virtual void OnRender(float deltaTime) {}; // Called after RTV + DSV clear, before internal DrawQueuedEvents
	virtual void OnPostRender(float deltaTime) {}; // Just before internal Present
	virtual void OnResize(int newWidth, int newHeight) {}; // Post internal Resize
	virtual void LoadContent() {}; // Load content to be used in Render, post Initialize
	virtual void UnloadContent() {}; // Unload content just before Destroy
	virtual void OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt) {};
	virtual void OnMouse(DirectX::Mouse::ButtonStateTracker mt, MouseData md, DirectX::Mouse::State state, float dt) {};

public:
	// Achilles drawing functions
	void QueueMeshDraw(std::shared_ptr<Mesh> mesh);
protected:
	void DrawMeshIndexed(ComPtr<ID3D12GraphicsCommandList2> commandList, std::shared_ptr<Mesh> mesh, std::shared_ptr<Camera> camera);
	void DrawQueuedEvents(ComPtr<ID3D12GraphicsCommandList2> commandList);
	void EmptyDrawQueue();
};