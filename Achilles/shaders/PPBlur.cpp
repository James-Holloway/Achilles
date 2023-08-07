#include "PPBlur.h"
#include "../Application.h"

static std::shared_ptr<Shader> PPBlurShader = nullptr;
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC PPBlurSignature{};
std::shared_ptr<Shader> PPBlur::GetPPBlurShader(ComPtr<ID3D12Device2> device)
{
    if (PPBlurShader.use_count() >= 1)
        return PPBlurShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 uavs(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    CD3DX12_DESCRIPTOR_RANGE1 srvs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterCB0].InitAsConstants(sizeof(BlurCB0) / 4, 0);
    rootParameters[RootParameters::RootParameterUAVs].InitAsDescriptorTable(1, &uavs);
    rootParameters[RootParameters::RootParameterSRVs].InitAsDescriptorTable(1, &srvs);

    PPBlurSignature.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(PPBlurSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    PPBlurShader = Shader::ShaderCS(device, rootSignature, L"PPBlur");

    return PPBlurShader;
}

static std::shared_ptr<Shader> PPBlurUpsample = nullptr;
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC PPBlurUpsampleSignature{};
std::shared_ptr<Shader> PPBlur::GetPPBlurUpsampleShader(ComPtr<ID3D12Device2> device)
{
    if (PPBlurUpsample.use_count() >= 1)
        return PPBlurUpsample;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 uavs(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    CD3DX12_DESCRIPTOR_RANGE1 srvs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterCB0].InitAsConstants(sizeof(BlurUpsampleCB0) / 4, 0);
    rootParameters[RootParameters::RootParameterUAVs].InitAsDescriptorTable(1, &uavs);
    rootParameters[RootParameters::RootParameterSRVs].InitAsDescriptorTable(1, &srvs);

    CD3DX12_STATIC_SAMPLER_DESC linearBorder(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    linearBorder.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

    PPBlurUpsampleSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &linearBorder, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(PPBlurUpsampleSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    PPBlurUpsample = Shader::ShaderCS(device, rootSignature, L"PPBlurUpsample");

    return PPBlurUpsample;
}

void PPBlur::BlurTexture(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> textures[2], std::shared_ptr<Texture>& lowerResBuf, float upsampleBlendFactor)
{
    auto device = Application::GetD3D12Device();
    // Set the shader constants
    float width = 0.0f;
    float height = 0.0f;
    textures[0]->GetSize(width, height);

    uint32_t bufferWidth = (uint32_t)width;
    uint32_t bufferHeight = (uint32_t)height;

    if (&textures[0] == &lowerResBuf) // Just blur
    {
        std::shared_ptr<Shader> shader = GetPPBlurShader(device);
        commandList->SetShader(shader);

        BlurCB0 blurCB0{};
        blurCB0.g_inverseDimensions = Vector2(1.0f / bufferWidth, 1.0f / bufferHeight);
        commandList->SetCompute32BitConstants<BlurCB0>(RootParameters::RootParameterCB0, blurCB0);
        // Set the input textures and output UAV
        commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 0, *textures[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 0, *textures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commandList->SetShader(shader);

        commandList->Dispatch2D(bufferWidth, bufferHeight);
    }
    else // Upsample and blur
    {
        std::shared_ptr<Shader> shader = GetPPBlurUpsampleShader(device);
        commandList->SetPipelineState(shader->pipelineState);
        commandList->SetComputeRootSignature(*shader->rootSignature);

        BlurUpsampleCB0 blurCB0{};
        blurCB0.g_inverseDimensions = Vector2(1.0f / bufferWidth, 1.0f / bufferHeight);
        blurCB0.g_upsampleBlendFactor = upsampleBlendFactor;
        commandList->SetCompute32BitConstants<BlurUpsampleCB0>(RootParameters::RootParameterCB0, blurCB0);
        // Set the input textures and output UAV
        commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 0, *textures[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 0, *textures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 1, *lowerResBuf, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commandList->SetShader(shader);

        commandList->Dispatch2D(bufferWidth, bufferHeight);
    }
    commandList->FlushResourceBarriers();
}
