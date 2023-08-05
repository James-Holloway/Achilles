#pragma once

#include "../ShaderInclude.h"

namespace StartupScreen
{
    struct Data
    {
        Color BackgroundColor;
        Vector2 UVOffset;
        Vector2 UVScale;
    };

    enum RootParameters
    {
        RootParameterData,
        RootParameterTextures,
        RootParameterCount
    };

    std::shared_ptr<Shader> GetStartupScreenShader(ComPtr<ID3D12Device2> device = nullptr);
};
