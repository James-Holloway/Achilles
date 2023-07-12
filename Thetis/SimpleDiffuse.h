#pragma once

#include "Achilles/ShaderInclude.h"
#include "CommonShader.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace SimpleDiffuse
{
    struct MaterialProperties
    {
        // 0 bytes
        Color Color;
        // 16 bytes

        MaterialProperties();
    };

    enum RootParameters
    {
        //// Vertex shader parameter ////
        RootParameterMatrices,  // ConstantBuffer<Matricies> MatricesCB : register(b0);
        RootParameterLightProperties, // ConstantBuffer<LightProperties> : register(b1)
        RootParameterAmbientLight, // ConstantBuffer<AmbientLight> : register(b2)
        RootParameterMaterialProperties, // ConstantBuffer<MaterialProperties> : register(b3)

        //// Pixel shader parameters ////
        RootParameterPointLights, // StructuredBuffer<PointLight> PointLights : register( t0 );
        RootParameterSpotLights, // StructuredBuffer<SpotLight> SpotLights : register( t1 );
        RootParameterDirectionalLights, // StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 )
        RootParameterTextures, // Texture2D DiffuseTexture : register( t3 );

        RootParameterCount
    };

    inline static std::shared_ptr<Texture> whitePixelTexture = nullptr;

    void SimpleDiffuseShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);
    std::shared_ptr<Mesh> SimpleDiffuseMeshCreation(aiScene* scene, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material);
    std::shared_ptr<Shader> GetSimpleDiffuseShader(ComPtr<ID3D12Device2> device = nullptr);
}