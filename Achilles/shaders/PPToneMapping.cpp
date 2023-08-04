#include "PPToneMapping.h"
#include "../Application.h"

using namespace PPToneMapping;

static std::shared_ptr<Shader> PPToneMappingShader = nullptr;
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC PPToneMappingSignature{};
std::shared_ptr<Shader> PPToneMapping::GetPPToneMappingShader(ComPtr<ID3D12Device2> device)
{
    if (PPToneMappingShader.use_count() >= 1)
        return PPToneMappingShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 uavs(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    CD3DX12_DESCRIPTOR_RANGE1 srvs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterCB0].InitAsConstants(sizeof(ToneMappingCB0) / 4, 0);
    rootParameters[RootParameters::RootParameterUAVs].InitAsDescriptorTable(1, &uavs);
    rootParameters[RootParameters::RootParameterSRVs].InitAsDescriptorTable(1, &srvs);

    PPToneMappingSignature.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(PPToneMappingSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    PPToneMappingShader = Shader::ShaderCS(device, rootSignature, L"PPToneMapping");

    return PPToneMappingShader;
}

void PPToneMapping::ApplyToneMapping(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture, std::shared_ptr<Texture> presentTexture, uint32_t ToneMapper)
{
    auto device = Application::GetD3D12Device();
    std::shared_ptr<Shader> toneMappingShader = GetPPToneMappingShader(device);

    float w = 0.0f;
    float h = 0.0f;
    texture->GetSize(w, h);
    uint32_t width = (uint32_t)w;
    uint32_t height = (uint32_t)h;

    ToneMappingCB0 toneMappingCB0;
    toneMappingCB0.RcpBufferDim = Vector2(1 / (float)width, 1 / (float)height);
    toneMappingCB0.ToneMapper = ToneMapper;

    commandList->SetPipelineState(toneMappingShader->pipelineState);
    commandList->SetComputeRootSignature(*toneMappingShader->rootSignature);

    commandList->SetCompute32BitConstants<ToneMappingCB0>(RootParameters::RootParameterCB0, toneMappingCB0);
    commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 0, *texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 0, *presentTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList->Dispatch2D(width, height);
    commandList->FlushResourceBarriers();
}

std::string PPToneMapping::ToneMappers::GetToneMapperName(uint32_t tonemapper)
{
    switch (tonemapper) {
    default:
    case ToneMappers::None:
        return "None";
    case ToneMappers::Clamp:
        return "Clamp";
    case ToneMappers::ExtendedReinhard:
        return "Extended Reinhard";
    case ToneMappers::Filmic:
        return "Filmic";
    }
    return "Unknown";
}
