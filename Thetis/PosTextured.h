#pragma once
#pragma once
#include "Achilles/ShaderInclude.h"

using DirectX::XMMATRIX;
using DirectX::XMFLOAT3;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Color;

namespace PosTextured
{
    struct PosTexturedVertex
    {
        Vector3 Position;
        Vector2 UV;
    };


    struct alignas(16) PosTexturedCB0
    {
        Matrix mvp;
    };

    static const PosTexturedVertex posTexturedQuadVertices[4] =
    {
        {Vector3(-1.0f, -1.0f, +0.0f), Vector2(0.0f, 0.0f)},
        {Vector3(+1.0f, -1.0f, +0.0f), Vector2(1.0f, 0.0f)},
        {Vector3(-1.0f, +1.0f, +0.0f), Vector2(0.0f, 1.0f)},
        {Vector3(+1.0f, +1.0f, +0.0f), Vector2(1.0f, 1.0f)},
    };

    static const uint16_t posTexturedQuadIndices[6] =
    {
        0, 1, 2,
        2, 1, 3
    };

    static const PosTexturedVertex posTexturedCubeVertices[8] = {
        { Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 0.0f) },
        { Vector3(-1.0f, +1.0f, -1.0f), Vector2(0.0f, 1.0f) },
        { Vector3(+1.0f, +1.0f, -1.0f), Vector2(1.0f, 1.0f) },
        { Vector3(+1.0f, -1.0f, -1.0f), Vector2(1.0f, 0.0f) },
        { Vector3(-1.0f, -1.0f, +1.0f), Vector2(0.0f, 0.0f) },
        { Vector3(-1.0f, +1.0f, +1.0f), Vector2(0.0f, 1.0f) },
        { Vector3(+1.0f, +1.0f, +1.0f), Vector2(1.0f, 1.0f) },
        { Vector3(+1.0f, -1.0f, +1.0f), Vector2(1.0f, 0.0f) }
    };

    static const uint16_t posTexturedCubeIndices[36] =
    {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    enum RootParameters
    {
        //// Vertex shader parameter ////
        RootParameterPosTexturedCB0,  // ConstantBuffer<PosTexturedCB0> MatCB : register(b0);

        //// Pixel shader parameters ////
        RootParameterTextures,
        // Texture2D MainTexture : register( t0 );

        RootParameterCount
    };

    static D3D12_INPUT_ELEMENT_DESC posTexturedInputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    void PosTexturedShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera);
    std::shared_ptr<Shader> GetPosTexturedShader(ComPtr<ID3D12Device2> device);
}