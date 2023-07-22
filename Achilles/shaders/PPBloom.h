#pragma once

#include "../ShaderInclude.h"
#include "PPBlur.h"

using DirectX::SimpleMath::Vector2;

namespace PPBloom
{
    enum RootParameters
    {
        RootParameterCB0,
        RootParameterUAVs,
        RootParameterSRVs,
        RootParameterCount
    };

    struct ExtractCB0
    {
        Vector2 g_inverseOutputSize;
        float g_bloomThreshold;
    };

    struct DownsampleCB0
    {
        Vector2 g_inverseDimensions;
    };

    struct ApplyCB0
    {
        Vector2 g_RcpBufferDim;
        float g_BloomStrength;
    };

    static std::shared_ptr<Texture> bloomBuffer1[2]; // bloomWidth
    static std::shared_ptr<Texture> bloomBuffer2[2]; // bloomWidth / 2
    static std::shared_ptr<Texture> bloomBuffer3[2]; // bloomWidth / 4
    static std::shared_ptr<Texture> bloomBuffer4[2]; // bloomWidth / 8
    static std::shared_ptr<Texture> bloomBuffer5[2]; // bloomWidth / 16

    static std::shared_ptr<Texture> bloomLumaBuffer; // bloomWidth

    static std::shared_ptr<Texture> bloomIntermediaryTexture; // Used as the output for the bloom apply stage, copied into the original texture

    std::shared_ptr<Shader> GetPPBloomExtractShader(ComPtr<ID3D12Device2> device);
    std::shared_ptr<Shader> GetPPBloomDownsampleShader(ComPtr<ID3D12Device2> device);
    std::shared_ptr<Shader> GetPPBloomApplyShader(ComPtr<ID3D12Device2> device);
    void CreateUAVs(float width, float height);
    void ResizeUAVs(float width, float height);
    void ApplyBloom(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture, float bloomThreshold, float upsampleBlendFactor, float bloomStrength);
}