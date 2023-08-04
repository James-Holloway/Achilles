#include "PPGammaCorrection.h"
#include "../Application.h"

using namespace PPGammaCorrection;

static std::shared_ptr<Shader> PPGammaCorrectionShader = nullptr;
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC PPGammaCorrectionSignature{};
std::shared_ptr<Shader> PPGammaCorrection::GetPPGammaCorrectionShader(ComPtr<ID3D12Device2> device)
{
    if (PPGammaCorrectionShader.use_count() >= 1)
        return PPGammaCorrectionShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 uavs(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterCB0].InitAsConstants(sizeof(GammaCorrectionCB0) / 4, 0);
    rootParameters[RootParameters::RootParameterUAVs].InitAsDescriptorTable(1, &uavs);

    PPGammaCorrectionSignature.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(PPGammaCorrectionSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    PPGammaCorrectionShader = Shader::ShaderCS(device, rootSignature, L"PPGammaCorrection");

    return PPGammaCorrectionShader;
}

void PPGammaCorrection::ApplyGammaCorrection(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> presentTexture, float GammaCorrection)
{
    auto device = Application::GetD3D12Device();
    std::shared_ptr<Shader> gammaCorrectionShader = GetPPGammaCorrectionShader(device);

    float w = 0.0f;
    float h = 0.0f;
    presentTexture->GetSize(w, h);
    uint32_t width = (uint32_t)w;
    uint32_t height = (uint32_t)h;

    GammaCorrectionCB0 gammaCorrectionCB0;
    gammaCorrectionCB0.RcpBufferDim = Vector2(1 / (float)width, 1 / (float)height);
    gammaCorrectionCB0.GammaCorrection = GammaCorrection;

    commandList->SetPipelineState(gammaCorrectionShader->pipelineState);
    commandList->SetComputeRootSignature(*gammaCorrectionShader->rootSignature);

    commandList->SetCompute32BitConstants<GammaCorrectionCB0>(RootParameters::RootParameterCB0, gammaCorrectionCB0);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 0, *presentTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList->Dispatch2D(width, height);
    commandList->FlushResourceBarriers();
}
