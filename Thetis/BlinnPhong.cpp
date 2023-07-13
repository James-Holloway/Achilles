#include "BlinnPhong.h"
#include "Achilles/Application.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace BlinnPhong;
using namespace CommonShader;

BlinnPhong::MaterialProperties::MaterialProperties() : Color(1, 1, 1, 1), Opacity(1), Diffuse(0.5f), Specular(0.5f), SpecularPower(1)
{

}

BlinnPhong::PixelInfo::PixelInfo() : CameraPosition(0, 0, 0), ShadingType(1)
{

}


void BlinnPhong::BlinnPhongShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData)
{
    CommonShaderMatrices matrices{};
    matrices.Model = object->GetWorldMatrix();
    matrices.View = camera->GetView();
    matrices.Projection = camera->GetProj();
    matrices.MVP = (matrices.Model * matrices.View) * matrices.Projection;
    matrices.InverseModel = object->GetInverseWorldMatrix();
    matrices.InverseView = camera->GetInverseView();
    commandList->SetGraphicsDynamicConstantBuffer<CommonShaderMatrices>(RootParameters::RootParameterMatrices, matrices);

    PixelInfo pixelInfo{};
    pixelInfo.CameraPosition = camera->position;
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

    commandList->SetGraphicsDynamicConstantBuffer<MaterialProperties>(RootParameters::RootParameterMaterialProperties, materialProperties);

    commandList->SetGraphicsDynamicStructuredBuffer<PointLight>(RootParameters::RootParameterPointLights, lightData.PointLights);
    commandList->SetGraphicsDynamicStructuredBuffer<SpotLight>(RootParameters::RootParameterSpotLights, lightData.SpotLights);
    commandList->SetGraphicsDynamicStructuredBuffer<DirectionalLight>(RootParameters::RootParameterDirectionalLights, lightData.DirectionalLights);

    std::shared_ptr<Texture> mainTexture = material.GetTexture(L"MainTexture");
    if (mainTexture != nullptr && mainTexture->IsValid())
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, mainTexture);
    else
        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, whitePixelTexture);
}

std::shared_ptr<Mesh> BlinnPhong::BlinnPhongMeshCreation(aiScene* scene, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material)
{
    // Create the vertices using the common shader vertex format
    std::shared_ptr<Mesh> mesh = CommonShaderMeshCreation(scene, inMesh, shader, material);

    if (scene->HasMaterials())
    {
        aiMaterial* mat = scene->mMaterials[inMesh->mMaterialIndex];
        material.name = StringToWString(mat->GetName().C_Str());

        // MainTexture - Diffuse or base colour
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString texturePath;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
            std::wstring textureFilename = std::filesystem::path(texturePath.C_Str()).filename().replace_extension();

            std::shared_ptr<Texture> texture = std::make_shared<Texture>();
            Object::GetCreationCommandList()->LoadTextureFromContent(*texture, textureFilename, TextureUsage::Diffuse);
            material.SetTexture(L"MainTexture", texture);
        }
        else if (mat->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
        {
            aiString texturePath;
            mat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &texturePath);
            std::wstring textureFilename = std::filesystem::path(texturePath.C_Str()).filename().replace_extension();

            std::shared_ptr<Texture> texture = std::make_shared<Texture>();
            Object::GetCreationCommandList()->LoadTextureFromContent(*texture, textureFilename, TextureUsage::Diffuse);
            material.SetTexture(L"MainTexture", texture);
        }

        // Color
        aiColor3D rgb;
        ai_real a;
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, rgb);
        mat->Get(AI_MATKEY_OPACITY, a);
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
        material.SetFloat(L"SpecularPower", specularPower);
    }
    else
    {
        material.SetFloat(L"Diffuse", 0.5f);
        material.SetFloat(L"Specular", 0.5f);
        material.SetFloat(L"SpecularPower", 32.0f);
    }

    material.SetFloat(L"ShadingType", 1);

    return mesh;
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

    // Textures descriptor range
    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0); // 1 texture, offset at 3, in space 0

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

    rootParameters[RootParameters::RootParameterPixelInfo].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterMaterialProperties].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterLightProperties].InitAsConstantBufferView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterAmbientLight].InitAsConstantBufferView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

    rootParameters[RootParameters::RootParameterPointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterSpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[RootParameters::RootParameterDirectionalLights].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // Sampler(s)
    CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // anisotropic sampler set to 8

    // Root signature creation
    BlinnPhongRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags); // 1 is number of static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(BlinnPhongRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    BlinnPhongShader = Shader::ShaderVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, BlinnPhongShaderRender, L"BlinnPhong");
    BlinnPhongShader->meshCreateCallback = BlinnPhongMeshCreation;

    if (whitePixelTexture == nullptr || !whitePixelTexture->IsValid())
    {
        whitePixelTexture = std::make_shared<Texture>();
        std::shared_ptr<CommandQueue> commandQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
        std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();
        std::vector<uint32_t> pixels = { 0xFFFFFFFF };
        commandList->CreateTextureFromMemory(*whitePixelTexture, L"White", pixels, 1, 1, TextureUsage::Albedo, false);
        commandQueue->ExecuteCommandList(commandList);
    }

    return BlinnPhongShader;
}
