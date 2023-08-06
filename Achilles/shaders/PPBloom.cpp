#include "PPBloom.h"
#include "../Application.h"

using namespace PPBloom;

static std::shared_ptr<Shader> PPBloomExtractShader = nullptr;
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC PPBloomExtractRootSignature{};
std::shared_ptr<Shader> PPBloom::GetPPBloomExtractShader(ComPtr<ID3D12Device2> device)
{
    if (PPBloomExtractShader.use_count() >= 1)
        return PPBloomExtractShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 uavs(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    CD3DX12_DESCRIPTOR_RANGE1 srvs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterCB0].InitAsConstants(sizeof(ExtractCB0) / 4, 0);
    rootParameters[RootParameters::RootParameterUAVs].InitAsDescriptorTable(1, &uavs);
    rootParameters[RootParameters::RootParameterSRVs].InitAsDescriptorTable(1, &srvs);

    CD3DX12_STATIC_SAMPLER_DESC biLinearClamp(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    PPBloomExtractRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &biLinearClamp, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(PPBloomExtractRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    PPBloomExtractShader = Shader::ShaderCS(device, rootSignature, L"PPBloomExtract");

    return PPBloomExtractShader;
}

static std::shared_ptr<Shader> PPBloomDownsampleShader = nullptr;
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC PPBloomDownsampleRootSignature{};
std::shared_ptr<Shader> PPBloom::GetPPBloomDownsampleShader(ComPtr<ID3D12Device2> device)
{
    if (PPBloomDownsampleShader.use_count() >= 1)
        return PPBloomDownsampleShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 uavs(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0);
    CD3DX12_DESCRIPTOR_RANGE1 srvs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterCB0].InitAsConstants(sizeof(DownsampleCB0) / 4, 0);
    rootParameters[RootParameters::RootParameterUAVs].InitAsDescriptorTable(1, &uavs);
    rootParameters[RootParameters::RootParameterSRVs].InitAsDescriptorTable(1, &srvs);

    CD3DX12_STATIC_SAMPLER_DESC biLinearClamp(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    PPBloomDownsampleRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &biLinearClamp, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(PPBloomDownsampleRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    PPBloomDownsampleShader = Shader::ShaderCS(device, rootSignature, L"PPBloomDownsample");

    return PPBloomDownsampleShader;
}

static std::shared_ptr<Shader> PPBloomApplyShader = nullptr;
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC PPBloomApplyRootSignature{};
std::shared_ptr<Shader> PPBloom::GetPPBloomApplyShader(ComPtr<ID3D12Device2> device)
{
    if (PPBloomApplyShader.use_count() >= 1)
        return PPBloomApplyShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 uavs(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0);
    CD3DX12_DESCRIPTOR_RANGE1 srvs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterCB0].InitAsConstants(sizeof(ApplyCB0) / 4, 0);
    rootParameters[RootParameters::RootParameterUAVs].InitAsDescriptorTable(1, &uavs);
    rootParameters[RootParameters::RootParameterSRVs].InitAsDescriptorTable(1, &srvs);

    CD3DX12_STATIC_SAMPLER_DESC linearClamp(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    PPBloomApplyRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &linearClamp, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(PPBloomApplyRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    PPBloomApplyShader = Shader::ShaderCS(device, rootSignature, L"PPBloomApply");

    return PPBloomApplyShader;
}


static bool hasCreatedUAVs = false;
void PPBloom::CreateUAVs(float width, float height)
{
    if (hasCreatedUAVs)
    {
        ResizeUAVs(width, height);
        return;
    }

    uint32_t bloomWidth = width > 2560 ? 1280 : 640;
    uint32_t bloomHeight = height > 1440 ? 768 : 384;

    DXGI_SAMPLE_DESC sampleDesc = { 1,0 };

    CD3DX12_RESOURCE_DESC resDesc{};
    resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, bloomWidth, bloomHeight, 1, 1, sampleDesc.Count, sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    bloomBuffer1[0] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 1a");
    bloomBuffer1[1] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 1b");

    resDesc.Width /= 2;
    resDesc.Height /= 2;
    bloomBuffer2[0] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 2a");
    bloomBuffer2[1] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 2b");

    resDesc.Width /= 2;
    resDesc.Height /= 2;
    bloomBuffer3[0] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 3a");
    bloomBuffer3[1] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 3b");

    resDesc.Width /= 2;
    resDesc.Height /= 2;
    bloomBuffer4[0] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 4a");
    bloomBuffer4[1] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 4b");

    resDesc.Width /= 2;
    resDesc.Height /= 2;
    bloomBuffer5[0] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 5a");
    bloomBuffer5[1] = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::Albedo, L"Bloom Buffer 5b");

    CD3DX12_RESOURCE_DESC lumaDesc{};
    lumaDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, (UINT64)bloomWidth, (UINT)bloomHeight, 1, 1, sampleDesc.Count, sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    bloomLumaBuffer = std::make_shared<Texture>(lumaDesc, nullptr, TextureUsage::Linear, L"Bloom Luma Buffer");

    CD3DX12_RESOURCE_DESC intermediaryDesc{};
    intermediaryDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, (UINT64)width, (UINT)height, 1, 1, sampleDesc.Count, sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    bloomIntermediaryTexture = std::make_shared<Texture>(intermediaryDesc, nullptr, TextureUsage::Albedo, L"Bloom Intermediary Texture");

    hasCreatedUAVs = true;
}

void PPBloom::ResizeUAVs(uint32_t width, uint32_t height)
{
    uint32_t bloomWidth = width > 2560 ? 1280 : 640;
    uint32_t bloomHeight = height > 1440 ? 768 : 384;

    bloomLumaBuffer->Resize(bloomWidth, bloomHeight);

    bloomBuffer1[0]->Resize(bloomWidth, bloomHeight);
    bloomBuffer1[1]->Resize(bloomWidth, bloomHeight);

    bloomWidth /= 2;
    bloomHeight /= 2;
    bloomBuffer2[0]->Resize(bloomWidth, bloomHeight);
    bloomBuffer2[1]->Resize(bloomWidth, bloomHeight);

    bloomWidth /= 2;
    bloomHeight /= 2;
    bloomBuffer3[0]->Resize(bloomWidth, bloomHeight);
    bloomBuffer3[1]->Resize(bloomWidth, bloomHeight);

    bloomWidth /= 2;
    bloomHeight /= 2;
    bloomBuffer4[0]->Resize(bloomWidth, bloomHeight);
    bloomBuffer4[1]->Resize(bloomWidth, bloomHeight);

    bloomWidth /= 2;
    bloomHeight /= 2;
    bloomBuffer5[0]->Resize(bloomWidth, bloomHeight);
    bloomBuffer5[1]->Resize(bloomWidth, bloomHeight);

    bloomIntermediaryTexture->Resize(width, height);
}

void PPBloom::ApplyBloom(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture, float bloomThreshold, float upsampleBlendFactor, float bloomStrength)
{
    auto device = Application::GetD3D12Device();
    std::shared_ptr<Shader> bloomExtract = GetPPBloomExtractShader(device);
    std::shared_ptr<Shader> bloomDownsample = GetPPBloomDownsampleShader(device);
    std::shared_ptr<Shader> bloomApply = GetPPBloomApplyShader(device);

    // Get size of bloom buffer 1
    float width = 0.0f;
    float height = 0.0f;
    bloomLumaBuffer->GetSize(width, height);

    uint32_t bloomWidth = (uint32_t)width;
    uint32_t bloomHeight = (uint32_t)height;

    // Extract bloom from texture into bloomBuffer1[0]
    ExtractCB0 extractCB0;
    extractCB0.g_inverseOutputSize = Vector2(1.0f / width, 1.0f/ height);
    extractCB0.g_bloomThreshold = bloomThreshold;

    commandList->SetPipelineState(bloomExtract->pipelineState);
    commandList->SetComputeRootSignature(*bloomExtract->rootSignature);

    commandList->SetCompute32BitConstants<ExtractCB0>(RootParameters::RootParameterCB0, extractCB0);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 0, *bloomBuffer1[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 0, *texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList->Dispatch2D(bloomWidth, bloomHeight);

    commandList->FlushResourceBarriers();

    // Downsample bloom buffer

    DownsampleCB0 downsampleCB0;
    downsampleCB0.g_inverseDimensions = extractCB0.g_inverseOutputSize;

    commandList->SetPipelineState(bloomDownsample->pipelineState);
    commandList->SetComputeRootSignature(*bloomDownsample->rootSignature);

    commandList->SetCompute32BitConstants<DownsampleCB0>(RootParameters::RootParameterCB0, downsampleCB0);
    commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 0, *bloomBuffer1[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 0, *bloomBuffer2[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 1, *bloomBuffer3[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 2, *bloomBuffer4[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 3, *bloomBuffer5[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList->Dispatch2D(bloomWidth / 2, bloomHeight / 2);

    commandList->FlushResourceBarriers();

    // Blur then then upsame + blur 4 times
    PPBlur::BlurTexture(commandList, bloomBuffer5, bloomBuffer5[0], 1.0f); // normal blur
    PPBlur::BlurTexture(commandList, bloomBuffer4, bloomBuffer5[1], upsampleBlendFactor); // upsample blur
    PPBlur::BlurTexture(commandList, bloomBuffer3, bloomBuffer4[1], upsampleBlendFactor);
    PPBlur::BlurTexture(commandList, bloomBuffer2, bloomBuffer3[1], upsampleBlendFactor);
    PPBlur::BlurTexture(commandList, bloomBuffer1, bloomBuffer2[1], upsampleBlendFactor);

    // Apply the bloom to the texture
    float textureWidth = 0.0f;
    float textureHeight = 0.0f;
    texture->GetSize(textureWidth, textureHeight);

    ApplyCB0 applyCB0;
    applyCB0.g_RcpBufferDim = Vector2(1.0f / textureWidth, 1.0f / textureHeight);
    applyCB0.g_BloomStrength = bloomStrength;

    commandList->SetPipelineState(bloomApply->pipelineState);
    commandList->SetComputeRootSignature(*bloomApply->rootSignature);

    commandList->SetCompute32BitConstants<ApplyCB0>(RootParameters::RootParameterCB0, applyCB0);
    commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 0, *bloomBuffer1[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetShaderResourceView(RootParameters::RootParameterSRVs, 1, *texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 0, *bloomIntermediaryTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetUnorderedAccessView(RootParameters::RootParameterUAVs, 1, *bloomLumaBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList->Dispatch2D((uint32_t)textureWidth, (uint32_t)textureHeight);

    commandList->FlushResourceBarriers();

    // Copy the intermediary texture to the input texture
    commandList->CopyResource(*texture, *bloomIntermediaryTexture);
}
