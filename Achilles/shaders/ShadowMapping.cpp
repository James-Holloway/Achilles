#include "ShadowMapping.h"

using namespace ShadowMapping;
using namespace CommonShader;

ShadowMapping::ShadowMatrices::ShadowMatrices() : MVP()
{

}

static std::shared_ptr<Shader> ShadowMappingShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC ShadowMappingRootSignature{};
std::shared_ptr<Shader> ShadowMapping::GetShadowMappingShader(ComPtr<ID3D12Device2> device)
{
    if (ShadowMappingShader.use_count() >= 1)
        return ShadowMappingShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 textures = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset 0, space 0


    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstants(sizeof(ShadowMatrices) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &textures, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    ShadowMappingRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags); // 0 static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(ShadowMappingRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    ShadowMappingShader = Shader::ShaderDepthOnlyVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, L"ShadowMapping", 500); // Use low bias for perspective lights

    return ShadowMappingShader;
}

static std::shared_ptr<Shader> ShadowMappingHighBiasShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC ShadowMappingHighBiasRootSignature{};
std::shared_ptr<Shader> ShadowMapping::GetShadowMappingHighBiasShader(ComPtr<ID3D12Device2> device)
{
    if (ShadowMappingHighBiasShader.use_count() >= 1)
        return ShadowMappingHighBiasShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 textures = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset 0, space 0

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstants(sizeof(ShadowMatrices) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &textures, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    ShadowMappingHighBiasRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags); // 0 static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(ShadowMappingHighBiasRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    ShadowMappingHighBiasShader = Shader::ShaderDepthOnlyVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, L"ShadowMapping", 100000); // Use high bias for orthographic lights

    return ShadowMappingHighBiasShader;
}
