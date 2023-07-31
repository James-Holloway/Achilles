#pragma once

#include "../ShaderInclude.h"
#include "CommonShader.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace BlinnPhong
{
    struct MaterialProperties
    {
        // 0 bytes
        Color Color;
        // 16 bytes
        float Opacity;
        float Diffuse;
        float Specular;
        float SpecularPower;
        // 32 bytes
        float ReceivesShadows;
        float Padding[3];
        // 48 bytes

        MaterialProperties();
    };

    struct PixelInfo
    {
        // 0 bytes
        Vector3 CameraPosition;
        float ShadingType; // 0 = disabled, 1 = normal, 2 = only lighting
        // 16 bytes

        PixelInfo();
    };

    enum RootParameters
    {
        //// Vertex shader parameter ////
        RootParameterMatrices,  // ConstantBuffer<Matricies> MatricesCB : register(b0)
        RootParameterPixelInfo, // ConstantBuffer<PixelInfo> PixelInfoCB : register(b1)
        RootParameterMaterialProperties, // ConstantBuffer<MaterialProperties> : register(b2)
        RootParameterLightProperties, // ConstantBuffer<LightProperties> : register(b3)
        RootParameterAmbientLight, // ConstantBuffer<AmbientLight> : register(b4)

        //// Pixel shader parameters ////
        RootParameterPointLights, // StructuredBuffer<PointLight> PointLights : register( t0 );
        RootParameterSpotLights, // StructuredBuffer<SpotLight> SpotLights : register( t1 );
        RootParameterDirectionalLights, // StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 )
        RootParameterTextures, // Texture2D DiffuseTexture : register( t3 );
        RootParameterShadowCount, // ConstantBuffer<ShadowCount> ShadowCountCB : register(b0, space1);
        RootParameterShadows, // StructuredBuffer<ShadowInfo> Shadows : register( t0, space1 )
        RootParameterShadowMaps, // Texture2D ShadowMap0 : register( t1, space1 )

        RootParameterCount
    };

    inline static std::shared_ptr<Texture> whitePixelTexture = nullptr;

    bool BlinnPhongShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);
    std::shared_ptr<Mesh> BlinnPhongMeshCreation(aiScene* scene, aiNode* node, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material);
    std::shared_ptr<Shader> GetBlinnPhongShader(ComPtr<ID3D12Device2> device = nullptr);
}