#pragma once

#include "../ShaderInclude.h"
#include "CommonShader.h"
#include "../ShadowCamera.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace ShadowMapping
{
    struct ShadowMatrices
    {
        Matrix MVP;

        ShadowMatrices();
    };

    struct PointShadowMatrices
    {
        Matrix MVP;
        Matrix Model;

        Vector3 PointLightPosition;
        float PointLightDistance;
    };

    enum RootParameters
    {
        //// Vertex shader parameters ////
        RootParameterMatrices,  // ConstantBuffer<Matricies> MatricesCB : register(b0)

        //// Pixel shader parameters ////
        RootParameterTextures, // Texture2D AlphaTexture : register(t0);

        RootParameterCount
    };

    std::shared_ptr<Shader> GetShadowMappingShader(ComPtr<ID3D12Device2> device = nullptr);
    std::shared_ptr<Shader> GetShadowMappingHighBiasShader(ComPtr<ID3D12Device2> device = nullptr);
    std::shared_ptr<Shader> GetShadowMappingPointShader(ComPtr<ID3D12Device2> device = nullptr);

    void DrawObjectShadowDirectional(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, DirectionalLight directionalLight, std::shared_ptr<Shader> shader);
    void DrawObjectShadowDirectionalCascaded(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, DirectionalLight directionalLight, std::shared_ptr<Shader> shader, Matrix cascadeMatrix);
    void DrawObjectShadowSpot(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, SpotLight spotLight, std::shared_ptr<Shader> shader);
    void DrawObjectShadowPoint(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<Camera> shadowCamera, LightObject* lightObject, PointLight pointLight, std::shared_ptr<Shader> shader, Matrix directionMatrix, DirectX::BoundingFrustum frustum);
    // Renders Cascaded Shadow Maps for directional lights
    void DrawShadowDirectionalCascaded(std::shared_ptr<CommandList> commandList, std::vector<std::shared_ptr<Object>> shadowCastingObjects, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, DirectionalLight directionalLight, std::shared_ptr<Shader> shader);
    // Assumes shaders ShadowMappingShader and ShadowMappingHighBiasShader have been loaded elsewhere before calling this
    void RenderShadowScene(std::shared_ptr<CommandList> commandList, std::shared_ptr<ShadowCamera> shadowCamera, std::vector<std::shared_ptr<Object>> shadowCastingObjects);
}