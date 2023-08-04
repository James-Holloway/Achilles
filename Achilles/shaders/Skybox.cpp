#include "Skybox.h"

using namespace Skybox;
using namespace CommonShader;

Skybox::SkyboxInfo::SkyboxInfo() : HasTexture(0), LookDirection(0, 0, -1), PrimarySunEnable(0), PrimarySunDirection(0, 0, 0), PrimarySunSize(1.0), PrimarySunColor(1, 1, 1, 1), PrimarySunShineExponent(32), HideSunBehindHorizon(1), Debug(0), SkyColor(0.165f, 0.451f, 0.706f, 1.0f), UpSkyColor(0.157f, 0.337f, 0.643f, 1.0f), HorizonColor(0.459f, 0.745f, 0.910f, 1.0f), GroundColor(0.294f, 0.255f, 0.224f, 1.0f)
{

}

bool Skybox::SkyboxShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData)
{
    SkyboxInfo skyboxInfo{};

    skyboxInfo.Model = object->GetLocalMatrix();
    skyboxInfo.MVP = skyboxInfo.Model * Matrix::CreateScale(camera->farZ * 1.999f) * Matrix::CreateFromYawPitchRoll(camera->GetRotation()).Invert() * camera->GetProj();

    skyboxInfo.LookDirection = camera->GetInverseView().Backward();
    skyboxInfo.LookDirection.Normalize();

    // If we have a primary sun
    if (lightData.DirectionalLights.size() >= 1)
    {
        Vector3 direction = Vector3(lightData.DirectionalLights[0].DirectionWorldSpace);
        direction.Normalize();

        skyboxInfo.PrimarySunEnable = 1;
        skyboxInfo.PrimarySunDirection = direction;
        skyboxInfo.PrimarySunSize = material.GetFloat(L"PrimarySunSize");
        skyboxInfo.PrimarySunColor = lightData.DirectionalLights[0].Color;
        skyboxInfo.PrimarySunShineExponent = material.GetFloat(L"PrimarySunShineExponent");
        skyboxInfo.HideSunBehindHorizon = material.GetFloat(L"HideSunBehindHorizon");
    }

    if (material.HasTexture(L"Cubemap"))
    {
        std::shared_ptr<Texture> cubemap = material.GetTexture(L"Cubemap");
        if (cubemap && cubemap->IsValid())
        {
            D3D12_RESOURCE_DESC cubemapDesc = cubemap->GetD3D12ResourceDesc();

            if (cubemapDesc.DepthOrArraySize >= 6) // only if a cubemap
            {
                skyboxInfo.HasTexture = 1;

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
                srvDesc.Format = cubemapDesc.Format;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MipLevels = (UINT)-1;

                commandList->SetShaderResourceView(RootParameters::RootParameterCubemap, 0, *cubemap, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, 0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, &srvDesc);
            }
        }
    }
    else
    {
        skyboxInfo.SkyColor = material.GetVector(L"SkyColor");
        skyboxInfo.UpSkyColor = material.GetVector(L"UpSkyColor");
        skyboxInfo.HorizonColor = material.GetVector(L"HorizonColor");
        skyboxInfo.GroundColor = material.GetVector(L"GroundColor");
    }
    skyboxInfo.Debug = material.GetFloat(L"Debug");

    commandList->SetGraphicsDynamicConstantBuffer<SkyboxInfo>(RootParameterSkyboxInfo, skyboxInfo);
    return true;
}

std::shared_ptr<Mesh> Skybox::SkyboxMeshCreation(aiScene* scene, aiNode* node, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material, std::wstring meshPath)
{
    // Create the vertices using the common shader vertex format
    std::shared_ptr<Mesh> mesh = CommonShaderMeshCreation(scene, node, inMesh, shader, material);
    return mesh;
}

static std::shared_ptr<Shader> SkyboxShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC SkyboxRootSignature{};
std::shared_ptr<Shader> Skybox::GetSkyboxShader(ComPtr<ID3D12Device2> device)
{
    if (SkyboxShader.use_count() >= 1)
        return SkyboxShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 cubemap(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset 0, space 0

    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterSkyboxInfo].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[RootParameters::RootParameterCubemap].InitAsDescriptorTable(1, &cubemap);

    CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // anisotropic sampler set to 8

    // Root signature creation
    SkyboxRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(SkyboxRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    SkyboxShader = Shader::ShaderSkyboxVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, SkyboxShaderRender, L"Skybox");
    SkyboxShader->meshCreateCallback = SkyboxMeshCreation;

    return SkyboxShader;
}
