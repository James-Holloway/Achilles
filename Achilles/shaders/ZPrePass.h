#pragma once

#include "../ShaderInclude.h"
#include "CommonShader.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace ZPrePass
{
    struct ZPrePassMatrix
    {
        Matrix MVP;

        ZPrePassMatrix();
    };

    enum RootParameters
    {
        //// Vertex shader parameters ////
        RootParameterMatrices,  // ConstantBuffer<Matricies> MatricesCB : register(b0)

        RootParameterCount
    };

    std::shared_ptr<Shader> GetZPrePassShader(ComPtr<ID3D12Device2> device = nullptr);
}