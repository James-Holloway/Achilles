#pragma once

#include "../ShaderInclude.h"

namespace PPToneMapping {

    enum RootParameters
    {
        RootParameterCB0,
        RootParameterUAVs,
        RootParameterSRVs,
        RootParameterCount
    };

    struct ToneMappingCB0
    {
        Vector2 RcpBufferDim;
        uint32_t ToneMapper;
    };

    namespace ToneMappers
    {
        enum {
            None,
            Clamp,
            ExtendedReinhard,
            Filmic,

            Count
        };

        std::string GetToneMapperName(uint32_t tonemapper);
    }

    std::shared_ptr<Shader> GetPPToneMappingShader(ComPtr<ID3D12Device2> device);
    void ApplyToneMapping(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture, std::shared_ptr<Texture> presentTexture, uint32_t ToneMapper = ToneMappers::ExtendedReinhard);
}