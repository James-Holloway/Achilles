#pragma once

#include <functional>
#include <d3d12.h>
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>
#include "AchillesDrop.h"
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
#include "Scene.h"
#include "Lights.h"
#include "SpriteObject.h"
#include "LightObject.h"
#include "PostProcessing.h"

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
    uint32_t clientWidth = 1600;
    uint32_t clientHeight = 900;

    // DirectX 12 Objects
    ComPtr<ID3D12Device2> device;
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12CommandAllocator> commandAllocators[BufferCount];

    std::shared_ptr<Texture> backBuffers[BufferCount];
    std::shared_ptr<RenderTarget> renderTarget;
    std::shared_ptr<RenderTarget> swapChainRenderTarget;
    std::shared_ptr<RenderTarget> preloadedRenderTarget;
    std::shared_ptr<Texture> singleSampledTexture;
    std::shared_ptr<Texture> intermediatePresentTexture;

    std::shared_ptr<CommandQueue> directCommandQueue;
    std::shared_ptr<CommandQueue> computeCommandQueue;
    std::shared_ptr<CommandQueue> copyCommandQueue;

    UINT currentBackBufferIndex = 0;

    // Synchronization objects
    uint64_t fenceValues[BufferCount] = { 0 };
    uint64_t frameValues[BufferCount] = { 0 };

private:
    // Initialization properties
    bool isInitialized = false; // Stores init state
    std::atomic<bool> hasLoaded = false; // Stores whether content has been loaded
    std::thread loadContentThread; // Loads content in this thread
    bool isLoading = false; // Stores whether we are on the startup screen. Set to true at the end of Initialize, set to false once loadContentThread has been joined
    std::shared_ptr<Texture> startupTexture;

    // Update variables
    uint64_t frameCounter = 0;
    double elapsedSeconds = 0.0;
    std::chrono::high_resolution_clock clock;
    std::chrono::steady_clock::time_point prevUpdateClock;
    std::chrono::steady_clock::time_point prevRenderClock;

protected:
    uint64_t totalFrameCount = 0;
    double totalElapsedSeconds = 0;

    std::atomic<bool> isDestroying = false; // For use in LoadContent when the application is closed early. To check use the ACHILLES_IF_DESTROYING_RETURN() macro

public:
    // Public statics for internal use
    inline static std::map<HWND, Achilles*> instanceMapping = std::map<HWND, Achilles*>();

public:
    // Public variables
    FLOAT clearColor[4] = { 0.4f, 0.58f, 0.93f, 1.0f }; // Cornflower Blue
    double lastFPS = 0.0;
    bool postProcessingEnable = true;
    std::atomic<bool> acceptingFiles = false;
    std::vector<std::function<void(void)>> postPresentFunctions;

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
    bool frustumCulling = true;
    std::deque<DrawEvent> drawEventQueue{}; // Opaque Draw Queue
    std::deque<DrawEvent> drawEventQueueTransparent{}; // Transparent Draw Queue
    std::shared_ptr<AchillesImGui> achillesImGui;
    std::shared_ptr<Object> skydome;

    // Scene objects
    std::shared_ptr<Scene> mainScene;
    std::set<std::shared_ptr<Scene>> scenes {};

    // Drawing states
    LightData lightData{};
    std::shared_ptr<PostProcessing> postProcessing;

public:
    // Constructor and destructor functions
    Achilles(std::wstring _name = L"Achilles", uint32_t width = 1600, uint32_t height = 900);
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
    void UpdateRenderTargetViews();
    void UpdateDepthStencilView();
    std::shared_ptr<Texture> CreateRenderTargetTexture(std::wstring name = L"Render Target Texture");
    void UpdateMainRenderTarget();
    std::shared_ptr<Texture> CreateIntermediatePresentTexture();
    void UpdatePreLoadedRenderTarget();
    void UpdateSingleSampledTexture();
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);

    // Get functions
    std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
    std::shared_ptr<RenderTarget> GetCurrentRenderTarget() const;
    std::shared_ptr<RenderTarget> GetPreLoadedRenderTarget() const;
    std::shared_ptr<RenderTarget> GetSwapChainRenderTarget() const;
    std::shared_ptr<Texture> GetIntermediatePresentTexture() const;

    // Resource, command queue and command list functions
    void Flush();

public:
    // Public functions used internally
    LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
    // Protected Achilles functions
    void HandleKeyboard();
    void HandleMouse(int& mouseX, int& mouseY, int& scroll, DirectX::Mouse::State& state, MouseData& mouseData);
    void Present(std::shared_ptr<CommandQueue> commandQueue, std::shared_ptr<CommandList> commandList);
    void LoadVitalContent(); // Loads content required for the startup screen
    virtual std::wstring GetStartupTexture() const; // Returns the content name for the startup texture
    virtual DirectX::SimpleMath::Color GetStartupColor() const; // Returns the background color for the startup loading screen
    void virtual UnloadPreLoadedAssets();
    void LoadInternalContent();
    std::shared_ptr<Texture> ResolveToSingleSampledTexture(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture);
    virtual void ApplyPostProcessing(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture, std::shared_ptr<Texture> presentTexture);
    virtual void CallPostPresentFunctions();

public:
    // Achilles functions to run things
    void Update(); // Optional (call Run otherwise)
    void Render(); // Optional (call Run otherwise)
    virtual void PreLoadedUpdate(); // Used during the startup splash screen before content has loaded
    virtual void PreLoadedRender(); // Shows startup splash screen before content has loaded
    void Resize(uint32_t width, uint32_t height); // Optional (call Run otherwise)
    void SetFullscreen(bool fs);
    void Initialize();
    void Run();
    void Destroy(); // Optional (call Run otherwise)

public:
    // Achilles callbacks
    virtual void OnUpdate(float deltaTime) {}; // Post internal Update
    virtual void OnRender(float deltaTime) {}; // Called after RTV + DSV clear, before internal internal drawing calls. Queue draws here
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
    std::shared_ptr<Texture> CreateCubemap(uint32_t width = 2048, uint32_t height = 2048);

public:
    // Scene functions
    std::shared_ptr<Scene> GetMainScene();
    void AddScene(std::shared_ptr<Scene> scene);
    void RemoveScene(std::shared_ptr<Scene> scene);
    void DrawActiveScenes();
    // Also populates the light and shadow info for LightData
    void DrawShadowScenes(std::shared_ptr<CommandList> commandList);
    virtual void AddObjectToScene(std::shared_ptr<Object> object);

public:
    // Achilles drawing functions
    void QueueObjectDraw(std::shared_ptr<Object> object);
    void QueueSpriteObjectDraw(std::shared_ptr<Object> object);
    void QueueSceneDraw(std::shared_ptr<Scene> scene); // Already called by DrawActiveScenes for active scenes in scenes
    void ClearLightData(LightData& lightData);
    void ConstructLightPositions(std::vector<std::shared_ptr<Object>> flattenedScene, std::shared_ptr<Camera> camera);
    void DrawSkybox(std::shared_ptr<CommandList> commandList, LightData& lightData);
protected:
    void DrawObjectKnitIndexed(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Camera> camera);
    void DrawObjectIndexed(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<Camera> camera);
    void DrawSpriteIndexed(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<Camera> camera);
    void DrawObjectShadowDirectional(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, DirectionalLight directionalLight, std::shared_ptr<Shader> shader);
    void DrawObjectShadowSpot(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, SpotLight spotLight, std::shared_ptr<Shader> shader);
    void DrawQueuedEvents(std::shared_ptr<CommandList> commandList);
    void EmptyDrawQueue();

public:
    // Static functions
    static Achilles* GetAchillesInstance(HWND hWnd);

public:
    // Drop functions
    virtual void HandleDroppedFile(std::wstring file, std::shared_ptr<CommandList> commandList);
    virtual void HandleDroppedFiles(std::vector<std::wstring> files);
    virtual void LoadObjectFromFile(std::wstring path);
    virtual void LoadTextureFromFile(std::wstring path, std::shared_ptr<CommandList> commandList);
};

#define ACHILLES_IF_DESTROYING_RETURN() if (isDestroying) return;