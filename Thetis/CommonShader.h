#pragma once

#include "Achilles/ShaderInclude.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace CommonShader
{
    struct CommonShaderVertex
    {
        Vector3 Position;
        Vector3 Normal;
        Vector3 Tangent;
        Vector3 Bitangent;
        Vector2 UV;

        CommonShaderVertex();
    };

    struct CommonShaderMatrices
    {
        Matrix MVP; // Model * View * Projection, used for SV_Position
        Matrix MV; // Model * View, used for position viewspace
        Matrix InverseMV; // Inversed then transposed model view matrix
    };

    static D3D12_INPUT_ELEMENT_DESC CommonShaderInputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    std::shared_ptr<Mesh> CommonShaderMeshCreation(aiScene* scene, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material);
}