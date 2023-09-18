#pragma once

#include "../Achilles/ShaderInclude.h"
#include "../Achilles/shaders/CommonShader.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace Water
{
    struct Matrices
    {
        Matrix MVP;
        Matrix VP;
        Matrix Model;
        Matrix View;
        Matrix Projection;
        Matrix InverseView;
    };

    struct PixelInfo
    {
        Vector3 CameraPosition;
        float Padding;

        PixelInfo();
    };

    struct WaterInfo
    {
        float Time;
        float TimeScale;
        float WaveScale;
        float WaveHorizontalScale;

        WaterInfo();
    };

    enum RootParameters
    {
        //// Vertex shader parameter ////
        RootParameterMatrices,  // ConstantBuffer<Matricies> MatricesCB : register(b0);
        RootParameterPixelInfo, // ConstantBuffer<PixelInfo> PixelInfoCB : register(b1);
        RootParameterWaterInfo, // ConstantBuffer<WaterInfo> WaterInfoCB : register(b2);
        RootParameterLightProperties, // ConstantBuffer<LightProperties> : register(b3);
        RootParameterAmbientLight, // ConstantBuffer<AmbientLight> : register(b4);

        //// Pixel shader parameters ////
        RootParameterPointLights, // StructuredBuffer<PointLight> PointLights : register( t0 );
        RootParameterSpotLights, // StructuredBuffer<SpotLight> SpotLights : register( t1 );
        RootParameterDirectionalLights, // StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 );
        RootParameterCascadeInfos, // StructuredBuffer<CascadeInfo> CascadeInfos : register( t3 );
        RootParameterTextures, // Texture2D DiffuseTexture : register( t0, space1 );

        RootParameterCount
    };

    bool WaterShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);
    bool WaterIsKnitTransparent(std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material);
    std::shared_ptr<Shader> GetWaterShader(ComPtr<ID3D12Device2> device = nullptr);
}