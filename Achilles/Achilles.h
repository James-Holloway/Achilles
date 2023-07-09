#pragma once
#include "Common.h"
#include "Resource.h"
#include "Texture.h"
#include "Application.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ResourceStateTracker.h"
#include "RenderTarget.h"
#include "Camera.h"
#include "Material.h"
#include "Shader.h"
#include "Mesh.h"
#include "DrawEvent.h"
#include "MouseData.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "AchillesImGui.h"

using Microsoft::WRL::ComPtr;

static LRESULT CALLBACK AchillesWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Achilles
{
protected:
	static const uint8_t BufferCount = 3; // Number of swap chain back buffers
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
	ComPtr<ID3D12CommandAllocator> commandAllocators[BufferCount];

	std::shared_ptr<Texture> backBuffers[BufferCount];
	std::shared_ptr<RenderTarget> renderTarget;
	std::shared_ptr<RenderTarget> swapChainRenderTarget;

	std::shared_ptr<CommandQueue> directCommandQueue;
	std::shared_ptr<CommandQueue> computeCommandQueue;
	std::shared_ptr<CommandQueue> copyCommandQueue;

	UINT RTVDescriptorSize = 0;
	UINT currentBackBufferIndex = 0;

	// Synchronization objects
	uint64_t fenceValues[BufferCount] = {0};
	uint64_t frameValues[BufferCount] = {0};

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
	double lastFPS = 0.0;

protected:
	// Protected variables for internal use
	std::wstring name = L"Achilles";
	// Keyboard tracking
	std::unique_ptr<DirectX::Keyboard> keyboard;
	DirectX::Keyboard::KeyboardStateTracker keyboardTracker;
	std::unique_ptr<DirectX::Mouse> mouse;
	DirectX::Mouse::ButtonStateTracker mouseTracker;
	MouseData prevMouseData{};

	std::deque<double> historicalFrameTimes{};

	// Achilles drawing internals
	std::queue<DrawEvent> drawEventQueue;
	std::shared_ptr<AchillesImGui> achillesImGui;

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
	void UpdateRenderTargetViews();
	void UpdateDepthStencilView();
	std::shared_ptr<Texture> CreateRenderTargetTexture(std::wstring name = L"Render Target Texture");
	void UpdateMainRenderTarget();
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);

	// Get functions
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
	std::shared_ptr<RenderTarget> GetCurrentRenderTarget() const;
	std::shared_ptr<RenderTarget> GetSwapChainRenderTarget() const;

	// Resource, command queue and command list functions
	void Flush();

public:
	// Public functions used internally
	LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
	// Protected Achilles functions
	void HandleKeyboard();
	void HandleMouse(int& mouseX, int& mouseY, int& scroll, DirectX::Mouse::State& state);
	void Present(std::shared_ptr<CommandQueue> commandQueue, std::shared_ptr<CommandList> commandList);

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
	// Achilles functions for creating things
	std::shared_ptr<Texture> CreateTexture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr);
	std::shared_ptr<Texture> CreateTexture(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue = nullptr);

public:
	// Achilles drawing functions
	void QueueMeshDraw(std::shared_ptr<Mesh> mesh);
protected:
	void DrawMeshIndexed(std::shared_ptr<CommandList> commandList, std::shared_ptr<Mesh> mesh, std::shared_ptr<Camera> camera);
	void DrawQueuedEvents(std::shared_ptr<CommandList> commandList);
	void EmptyDrawQueue();
};