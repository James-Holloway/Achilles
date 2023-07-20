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

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstants(sizeof(ShadowMatrices) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    ShadowMappingRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags); // 0 static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(ShadowMappingRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    ShadowMappingShader = Shader::ShaderDepthOnlyVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, L"ShadowMapping");

    return ShadowMappingShader;
}
