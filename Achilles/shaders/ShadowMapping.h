#pragma once

#include "../ShaderInclude.h"
#include "CommonShader.h"

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
}