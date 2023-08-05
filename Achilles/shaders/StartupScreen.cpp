#include "StartupScreen.h"

using namespace StartupScreen;

static std::shared_ptr<Shader> StartupScreenShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC StartupScreenRootSignature{};
std::shared_ptr<Shader> StartupScreen::GetStartupScreenShader(ComPtr<ID3D12Device2> device)
{
    if (StartupScreenShader.use_count() >= 1)
    {
        return StartupScreenShader;
    }
    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset at 0, in space 0

    CD3DX12_STATIC_SAMPLER_DESC linearSampler(0, D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterData].InitAsConstants(sizeof(Data) / 4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

    StartupScreenRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &linearSampler, rootSignatureFlags);

    std::shared_ptr rootSignature = std::make_shared<RootSignature>(StartupScreenRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    StartupScreenShader = Shader::ShaderVSPS(device, nullptr, 0, 0, rootSignature, nullptr, L"StartupScreen", D3D12_CULL_MODE_NONE, false, DXGI_FORMAT_R8G8B8A8_UNORM);

    return StartupScreenShader;
}