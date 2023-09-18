#include "Water.h"
#include "Achilles/Application.h"

using namespace Water;
using namespace CommonShader;
using namespace DirectX;

Water::PixelInfo::PixelInfo() : CameraPosition(0, 0, 0), Padding{ 0 }
{

}

Water::WaterInfo::WaterInfo() : Time(0), TimeScale(1), WaveScale(1), WaveHorizontalScale(1)
{

}

bool Water::WaterShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData)
{
    ScopedTimer _prof(L"WaterShaderRender");
    Matrices matrices{};
    matrices.Model = object->GetWorldMatrix();
    matrices.View = camera->GetView();
    matrices.Projection = camera->GetProj();
    matrices.MVP = (matrices.Model * matrices.View) * matrices.Projection;
    matrices.VP = matrices.View * matrices.Projection;
    matrices.InverseView = camera->GetInverseView();
    commandList->SetGraphicsDynamicConstantBuffer<Matrices>(RootParameters::RootParameterMatrices, matrices);

    std::shared_ptr<Texture> noiseTexture = material.GetTexture(L"MainTexture");
    material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, noiseTexture);

    PixelInfo pixelInfo{};
    pixelInfo.CameraPosition = camera->GetPosition();

    commandList->SetGraphicsDynamicConstantBuffer<PixelInfo>(RootParameters::RootParameterPixelInfo, pixelInfo);

    WaterInfo waterInfo{};
    waterInfo.Time = Application::GetTimeElapsed();
    waterInfo.TimeScale = 1 / 30.0f;
    waterInfo.WaveScale = 1;
    waterInfo.WaveHorizontalScale = 1 / 50.0f;
    commandList->SetGraphicsDynamicConstantBuffer<WaterInfo>(RootParameters::RootParameterWaterInfo, waterInfo);

    commandList->SetGraphicsDynamicConstantBuffer<LightProperties>(RootParameters::RootParameterLightProperties, lightData.GetLightProperties());
    commandList->SetGraphicsDynamicConstantBuffer<AmbientLight>(RootParameters::RootParameterAmbientLight, lightData.AmbientLight);

    commandList->SetGraphicsDynamicStructuredBuffer<PointLight>(RootParameters::RootParameterPointLights, lightData.PointLights);
    commandList->SetGraphicsDynamicStructuredBuffer<SpotLight>(RootParameters::RootParameterSpotLights, lightData.SpotLights);
    commandList->SetGraphicsDynamicStructuredBuffer<DirectionalLight>(RootParameters::RootParameterDirectionalLights, lightData.DirectionalLights);

    return true;
}

bool Water::WaterIsKnitTransparent(std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material)
{
    return true;
}


static std::shared_ptr<Shader> WaterShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC WaterRootSignature{};
std::shared_ptr<Shader> Water::GetWaterShader(ComPtr<ID3D12Device2> device)
{
    if (WaterShader.use_count() >= 1)
        return WaterShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    // Texture descriptor ranges
    CD3DX12_DESCRIPTOR_RANGE1 textureDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1); // 1 texture, offset at 0, in space 1

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

    rootParameters[RootParameters::RootParameterPixelInfo].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterWaterInfo].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[RootParameters::RootParameterLightProperties].InitAsConstantBufferView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[RootParameters::RootParameterAmbientLight].InitAsConstantBufferView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

    rootParameters[RootParameters::RootParameterPointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[RootParameters::RootParameterSpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[RootParameters::RootParameterDirectionalLights].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[RootParameters::RootParameterCascadeInfos].InitAsShaderResourceView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &textureDescriptorRange, D3D12_SHADER_VISIBILITY_ALL);

    // Sampler(s)
    CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // anisotropic sampler set to 8

    // Root signature creation
    WaterRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(WaterRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    WaterShader = Shader::ShaderVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, WaterShaderRender, L"Water", D3D12_CULL_MODE_BACK, true);
    WaterShader->knitTransparencyCallback = WaterIsKnitTransparent;

    return WaterShader;
}