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
        // Possible TODO: Sample diffuse texture to clip out alpha < 0.1

        RootParameterCount
    };

    std::shared_ptr<Shader> GetShadowMappingShader(ComPtr<ID3D12Device2> device = nullptr);
}