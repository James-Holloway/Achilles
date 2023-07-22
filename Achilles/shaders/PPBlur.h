#pragma once

#include "../ShaderInclude.h"

namespace PPBlur {

    enum RootParameters
    {
        RootParameterCB0,
        RootParameterUAVs,
        RootParameterSRVs,
        RootParameterCount
    };

    struct BlurCB0
    {
        Vector2 g_inverseDimensions;
    };
    struct BlurUpsampleCB0
    {
        Vector2 g_inverseDimensions;
        float g_upsampleBlendFactor;
    };

    std::shared_ptr<Shader> GetPPBlurShader(ComPtr<ID3D12Device2> device);
    std::shared_ptr<Shader> GetPPBlurUpsampleShader(ComPtr<ID3D12Device2> device);

    // @param textures[1] is output
    void BlurTexture(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> textures[2], std::shared_ptr<Texture>& lowerResBuf, float upsampleBlendFactor);
}