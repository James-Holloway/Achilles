#pragma once

#include "../ShaderInclude.h"

namespace PPGammaCorrection
{

    enum RootParameters
    {
        RootParameterCB0,
        RootParameterUAVs,
        RootParameterCount
    };

    struct GammaCorrectionCB0
    {
        Vector2 RcpBufferDim;
        float GammaCorrection = 2.2f;
    };

    std::shared_ptr<Shader> GetPPGammaCorrectionShader(ComPtr<ID3D12Device2> device);
    void ApplyGammaCorrection(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> presentTexture, float GammaCorrection = 2.2f);
}