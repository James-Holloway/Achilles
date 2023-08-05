#include "BlinnPhong.h"
#include "../ShadowMap.h"
#include "../Application.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace BlinnPhong;
using namespace CommonShader;
using namespace DirectX;

BlinnPhong::MaterialProperties::MaterialProperties() : Color(1, 1, 1, 1), Opacity(1), Diffuse(0.5f), Specular(0.5f), SpecularPower(1), EmissionStrength(1), ReceivesShadows(1), IsTransparent(0), TextureFlags(0)
{

}

BlinnPhong::PixelInfo::PixelInfo() : CameraPosition(0, 0, 0), ShadingType(1)
{

}


bool BlinnPhong::BlinnPhongShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData)
{
    ScopedTimer _prof(L"BlinnPhongShaderRender");
    CommonShaderMatrices matrices{};
    matrices.Model = object->GetWorldMatrix();
    matrices.View = camera->GetView();
    matrices.Projection = camera->GetProj();
    matrices.MVP = (matrices.Model * matrices.View) * matrices.Projection;
    matrices.InverseModel = object->GetInverseWorldMatrix();
    matrices.InverseView = camera->GetInverseView();
    commandList->SetGraphicsDynamicConstantBuffer<CommonShaderMatrices>(RootParameters::RootParameterMatrices, matrices);

    PixelInfo pixelInfo{};
    pixelInfo.CameraPosition = camera->GetPosition();
    if (material.HasFloat(L"ShadingType"))
        pixelInfo.ShadingType = material.GetFloat(L"ShadingType");

    commandList->SetGraphicsDynamicConstantBuffer<PixelInfo>(RootParameters::RootParameterPixelInfo, pixelInfo);

    commandList->SetGraphicsDynamicConstantBuffer<LightProperties>(RootParameters::RootParameterLightProperties, lightData.GetLightProperties());
    commandList->SetGraphicsDynamicConstantBuffer<AmbientLight>(RootParameters::RootParameterAmbientLight, lightData.AmbientLight);

    MaterialProperties materialProperties{};
    if (material.HasVector(L"Color"))
        materialProperties.Color = material.GetVector(L"Color");
    if (material.HasFloat(L"Diffuse"))
        materialProperties.Diffuse = material.GetFloat(L"Diffuse");
    if (material.HasFloat(L"Specular"))
        materialProperties.Specular = material.GetFloat(L"Specular");
    if (material.HasFloat(L"SpecularPower"))
        materialProperties.SpecularPower = material.GetFloat(L"SpecularPower");
    if (material.HasFloat(L"EmissionStrength"))
        materialProperties.EmissionStrength = material.GetFloat(L"EmissionStrength");
    materialProperties.ReceivesShadows = object->ReceivesShadows();

    commandList->SetGraphicsDynamicStructuredBuffer<PointLight>(RootParameters::RootParameterPointLights, lightData.PointLights);
    commandList->SetGraphicsDynamicStructuredBuffer<SpotLight>(RootParameters::RootParameterSpotLights, lightData.SpotLights);
    commandList->SetGraphicsDynamicStructuredBuffer<DirectionalLight>(RootParameters::RootParameterDirectionalLights, lightData.DirectionalLights);
    commandList->SetGraphicsDynamicStructuredBuffer<LightInfo>(RootParameters::RootParameterLightInfos, lightData.SortedLightInfo);

    std::shared_ptr<Texture> mainTexture = material.GetTexture(L"MainTexture");
    if (mainTexture != nullptr && mainTexture->IsValid())
    {
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, mainTexture);
        materialProperties.TextureFlags |= TextureFlags::Diffuse;
    }
    else
    {
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, whitePixelTexture);
    }

    std::shared_ptr<Texture> normalTexture = material.GetTexture(L"NormalTexture");
    if (normalTexture != nullptr && normalTexture->IsValid())
    {
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 1, normalTexture);
        materialProperties.TextureFlags |= TextureFlags::Normal;
    }
    else
    {
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 1, nullptr);
    }

    std::shared_ptr<Texture> emissionTexture = material.GetTexture(L"EmissionTexture");
    if (emissionTexture != nullptr && emissionTexture->IsValid())
    {
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 2, emissionTexture);
        materialProperties.TextureFlags |= TextureFlags::Emission;
    }
    else
    {
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 2, nullptr);
    }

    // Pass material properties now that we have the textue information
    commandList->SetGraphicsDynamicConstantBuffer<MaterialProperties>(RootParameters::RootParameterMaterialProperties, materialProperties);

    // Pass shadow maps to the shader for Shadow Factor

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = ShadowMap::GetShadowMapR32SRV();

    for (int i = 0; i < MAX_SHADOW_MAPS; i++)
    {
        std::shared_ptr<Texture> shadowMap = nullptr;
        if (i < lightData.SortedShadowMaps.size())
            shadowMap = lightData.SortedShadowMaps[i];

        if (shadowMap == nullptr)
            material.shader->BindTexture(*commandList, RootParameters::RootParameterShadowMaps, i, nullptr);
        else
            commandList->SetShaderResourceView(RootParameters::RootParameterShadowMaps, i, *shadowMap, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, 0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, &srvDesc);
    }

    return true;
}

std::shared_ptr<Mesh> BlinnPhong::BlinnPhongMeshCreation(aiScene* scene, aiNode* node, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material, std::wstring meshPath)
{
    // Create the vertices using the common shader vertex format
    std::shared_ptr<Mesh> mesh = CommonShaderMeshCreation(scene, node, inMesh, shader, material, meshPath);

    if (scene->HasMaterials())
    {
        aiMaterial* mat = scene->mMaterials[inMesh->mMaterialIndex];
        material.name = StringToWString(mat->GetName().C_Str());

        // MainTexture - Diffuse or base colour
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0 || mat->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
        {
            aiString texturePath;
            if (mat->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
                mat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &texturePath);
            if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) // If we have both then use diffuse
                mat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
            
            std::filesystem::path path = std::filesystem::path(texturePath.C_Str());
            std::wstring textureFilename = path.filename().replace_extension();

            const aiTexture* tex = scene->GetEmbeddedTexture(texturePath.C_Str());
            if (tex != nullptr) // Texture exists as embedded texture
            {
                std::shared_ptr<Texture> texture = Texture::LoadTextureFromAssimp(Object::GetCreationCommandList(), tex, textureFilename);
                material.SetTexture(L"MainTexture", texture);
            }
            else // Texture is just a filename
            {
                std::wstring basePath = std::filesystem::path(meshPath).remove_filename();
                std::shared_ptr<Texture> texture = Texture::GetTextureFromPath(Object::GetCreationCommandList(), std::filesystem::path(texturePath.C_Str()), basePath);
                material.SetTexture(L"MainTexture", texture);
            }
        }

        // NormalTexture
        if (mat->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            aiString texturePath;
            mat->GetTexture(aiTextureType_NORMALS, 0, &texturePath);

            std::filesystem::path path = std::filesystem::path(texturePath.C_Str());
            std::wstring textureFilename = path.filename().replace_extension();

            const aiTexture* tex = scene->GetEmbeddedTexture(texturePath.C_Str());
            if (tex != nullptr) // Texture exists as embedded texture
            {
                std::shared_ptr<Texture> texture = Texture::LoadTextureFromAssimp(Object::GetCreationCommandList(), tex, textureFilename);
                material.SetTexture(L"NormalTexture", texture);
            }
            else // Texture is just a filename
            {
                std::wstring basePath = std::filesystem::path(meshPath).remove_filename();
                std::shared_ptr<Texture> texture = Texture::GetTextureFromPath(Object::GetCreationCommandList(), std::filesystem::path(texturePath.C_Str()), basePath);
                material.SetTexture(L"NormalTexture", texture);
            }
        }

        // EmissionTexture
        if (mat->GetTextureCount(aiTextureType_EMISSIVE) > 0)
        {
            aiString texturePath;
            mat->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath);

            std::filesystem::path path = std::filesystem::path(texturePath.C_Str());
            std::wstring textureFilename = path.filename().replace_extension();

            const aiTexture* tex = scene->GetEmbeddedTexture(texturePath.C_Str());
            if (tex != nullptr) // Texture exists as embedded texture
            {
                std::shared_ptr<Texture> texture = Texture::LoadTextureFromAssimp(Object::GetCreationCommandList(), tex, textureFilename);
                material.SetTexture(L"EmissionTexture", texture);
            }
            else // Texture is just a filename
            {
                std::wstring basePath = std::filesystem::path(meshPath).remove_filename();
                std::shared_ptr<Texture> texture = Texture::GetTextureFromPath(Object::GetCreationCommandList(), std::filesystem::path(texturePath.C_Str()), basePath);
                material.SetTexture(L"EmissionTexture", texture);
            }
        }

        // Color
        aiColor3D rgb;
        ai_real a;
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, rgb);
        mat->Get(AI_MATKEY_OPACITY, a);
        if (a < 0)
            a = (ai_real)1.0f;

        Vector4 color(rgb.r, rgb.g, rgb.b, (float)a);
        material.SetVector(L"Color", color);

        /*OutputDebugStringAFormatted("\n\n%s:\n", mat->GetName().C_Str());
        for (uint32_t i = 0; i < mat->mNumProperties; i++)
        {
            aiMaterialProperty* prop = mat->mProperties[i];
            OutputDebugStringAFormatted("Material Key: %s\n", prop->mKey.C_Str());
            if (prop->mType == aiPropertyTypeInfo::aiPTI_Float)
            {
                OutputDebugStringAFormatted("%.2f\n", *((float*)prop->mData));
            }
        }*/

        // Set material floats
        ai_real specular = 0.0f;
        mat->Get(AI_MATKEY_SHININESS_STRENGTH, specular);
        material.SetFloat(L"Diffuse", 1.0f - (float)specular);
        material.SetFloat(L"Specular", (float)specular);

        ai_real specularPower = 32.0f;
        mat->Get(AI_MATKEY_SHININESS, specularPower);
        if (specularPower == 0.0f)
            specularPower = 32.0f;

        material.SetFloat(L"SpecularPower", specularPower);
    }
    else
    {
        material.SetFloat(L"Diffuse", 0.5f);
        material.SetFloat(L"Specular", 0.5f);
        material.SetFloat(L"SpecularPower", 32.0f);
    }

    material.SetFloat(L"EmissionStrength", 1.0f);
    material.SetFloat(L"ShadingType", 1);

    return mesh;
}

bool BlinnPhong::BlinnPhongIsKnitTransparent(std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material)
{
    std::shared_ptr<Texture> texture = material.GetTexture(L"MainTexture");
    if (texture == nullptr || !texture->IsValid())
        return false;

    Color color = material.GetVector(L"Color");

    return texture->IsTransparent() || color.w < 1.0f;
}

static std::shared_ptr<Shader> BlinnPhongShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC BlinnPhongRootSignature{};
std::shared_ptr<Shader> BlinnPhong::GetBlinnPhongShader(ComPtr<ID3D12Device2> device)
{
    if (BlinnPhongShader.use_count() >= 1)
        return BlinnPhongShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    // Texture descriptor ranges
    CD3DX12_DESCRIPTOR_RANGE1 textureDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 4, 0); // 3 textures, offset at 4, in space 0
    CD3DX12_DESCRIPTOR_RANGE1 shadowDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0, 1); // 8 textures, offset at 0, in space 1

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

    rootParameters[RootParameters::RootParameterPixelInfo].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterMaterialProperties].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterLightProperties].InitAsConstantBufferView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[RootParameters::RootParameterAmbientLight].InitAsConstantBufferView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

    rootParameters[RootParameters::RootParameterPointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterSpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterDirectionalLights].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterLightInfos].InitAsShaderResourceView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);

    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &textureDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterShadowMaps].InitAsDescriptorTable(1, &shadowDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // Sampler(s)
    std::vector<CD3DX12_STATIC_SAMPLER_DESC> samplers;
    CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // anisotropic sampler set to 8
    samplers.push_back(anisotropicSampler);

    CD3DX12_STATIC_SAMPLER_DESC trilinearSampler = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // trilinear sampler
    samplers.push_back(trilinearSampler);

    CD3DX12_STATIC_SAMPLER_DESC shadowSampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    shadowSampler.RegisterSpace = 1;
    shadowSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers.push_back(shadowSampler);

    // Root signature creation
    BlinnPhongRootSignature.Init_1_1(_countof(rootParameters), rootParameters, (UINT)samplers.size(), samplers.data(), rootSignatureFlags);
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(BlinnPhongRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    BlinnPhongShader = Shader::ShaderVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, BlinnPhongShaderRender, L"BlinnPhong", D3D12_CULL_MODE_BACK, true);
    BlinnPhongShader->meshCreateCallback = BlinnPhongMeshCreation;
    BlinnPhongShader->knitTransparencyCallback = BlinnPhongIsKnitTransparent;

    whitePixelTexture = Texture::GetCachedTexture(L"White");

    return BlinnPhongShader;
}
