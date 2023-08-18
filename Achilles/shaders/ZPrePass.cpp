#include "ZPrePass.h"

using namespace ZPrePass;
using namespace CommonShader;

ZPrePass::ZPrePassMatrix::ZPrePassMatrix() : MVP()
{

}

static std::shared_ptr<Shader> ZPrePassShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC ZPrePassRootSignature{};
std::shared_ptr<Shader> ZPrePass::GetZPrePassShader(ComPtr<ID3D12Device2> device)
{
    if (ZPrePassShader.use_count() >= 1)
        return ZPrePassShader;

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
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstants(sizeof(ZPrePassMatrix) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    ZPrePassRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags); // 1 static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(ZPrePassRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    ZPrePassShader = Shader::ShaderDepthOnlyVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, L"ZPrePass", 1);

    return ZPrePassShader;
}