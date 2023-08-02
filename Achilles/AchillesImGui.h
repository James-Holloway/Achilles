#pragma once

#include <memory>
#include <vector>
#pragma warning (push)
#pragma warning (disable : 26451)
#include "imgui.h"
#include "implot.h"
#pragma warning (pop)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class CommandList;
class RenderTarget;
class RootSignature;
class ShaderResourceView;
class Texture;

class AchillesImGui
{
    friend class Achilles;
protected:
    ComPtr<ID3D12Device2> device;
    HWND hWnd;
    ImGuiContext* imGuiContext;
    ImPlotContext* imPlotContext;
    std::shared_ptr<Texture> fontTexture;
    std::shared_ptr<ShaderResourceView> fontSRV;
    std::shared_ptr<RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;

    inline static std::vector<std::shared_ptr<Texture>> texturesInUse{};

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

    static void Image(std::shared_ptr<Texture> texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0))
    {
        texturesInUse.push_back(texture);
        ImGui::Image((ImTextureID)(size_t)texture.get(), size, uv0, uv1, tint_col, border_col);
    }

    static bool ImageButton(std::shared_ptr<Texture> texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
    {
        texturesInUse.push_back(texture);
        return ImGui::ImageButton((ImTextureID)(size_t)texture.get(), size, uv0, uv1, frame_padding, bg_col, tint_col);
    }
};