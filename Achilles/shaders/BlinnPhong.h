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
        Vector4 UVScaleOffset;
        // 32 bytes
        float Opacity;
        float Diffuse;
        float Specular;
        float SpecularPower;
        // 48 bytes
        float EmissionStrength;
        float ReceivesShadows;
        float IsTransparent;
        uint32_t TextureFlags;
        // 64 bytes

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
        RootParameterMatrices,  // ConstantBuffer<Matricies> MatricesCB : register(b0);
        RootParameterPixelInfo, // ConstantBuffer<PixelInfo> PixelInfoCB : register(b1);
        RootParameterMaterialProperties, // ConstantBuffer<MaterialProperties> : register(b2);
        RootParameterLightProperties, // ConstantBuffer<LightProperties> : register(b3);
        RootParameterAmbientLight, // ConstantBuffer<AmbientLight> : register(b4);

        //// Pixel shader parameters ////
        RootParameterPointLights, // StructuredBuffer<PointLight> PointLights : register( t0 );
        RootParameterSpotLights, // StructuredBuffer<SpotLight> SpotLights : register( t1 );
        RootParameterDirectionalLights, // StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 );
        RootParameterCascadeInfos, // StructuredBuffer<CascadeInfo> CascadeInfos : register( t3 );
        RootParameterTextures, // Texture2D DiffuseTexture : register( t0, space1 );
        RootParameterSpotShadowMaps, // Texture2D SpotShadowMap0 : register(t0, space2);
        RootParameterCascadeShadowMaps, // Texture2D CascadedShadowMap0[8] : register(t0, space3);
        RootParameterPointShadowMaps, // TextureCube PointShadowMap0 : register(t0, space4);

        RootParameterCount
    };

    inline static std::shared_ptr<Texture> whitePixelTexture = nullptr;

    bool BlinnPhongShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);
    std::shared_ptr<Mesh> BlinnPhongMeshCreation(aiScene* scene, aiNode* node, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material, std::wstring meshPath);
    bool BlinnPhongIsKnitTransparent(std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material);
    std::shared_ptr<Shader> GetBlinnPhongShader(ComPtr<ID3D12Device2> device = nullptr);
}