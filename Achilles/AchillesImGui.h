#pragma once

#pragma warning (push)
#pragma warning (disable : 26451)
#include "imgui.h"
#include "implot.h"
#pragma warning (pop)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class CommandList;
class RenderTarget;
class RootSignature;
class ShaderResourceView;
class Texture;

class AchillesImGui
{
    friend class Achilles;
public:
    LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Called at the start of the frame
    void NewFrame();

    // Renders ImGui to the RT
    void Render(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget);

    // Destroys the ImGui context
    void Destroy();

    // Set the font scaling for ImGui, this should only be called when the window's DPI scaling changes
    void SetScaling(float scale);

    // Setup the style for AchillesImGui
    virtual void SetupStyle();

    AchillesImGui(ComPtr<ID3D12Device2> _device, HWND _hWnd, const RenderTarget& renderTarget);
    virtual ~AchillesImGui();

protected:
    ComPtr<ID3D12Device2> device;
    HWND hWnd;
    ImGuiContext* imGuiContext;
    ImPlotContext* imPlotContext;
    std::shared_ptr<Texture> fontTexture;
    std::shared_ptr<ShaderResourceView> fontSRV;
    std::shared_ptr<RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
};