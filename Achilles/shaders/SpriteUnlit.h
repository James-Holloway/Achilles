#pragma once

#include "../ShaderInclude.h"
#include "CommonShader.h"
#include "../SpriteCommon.h"

namespace SpriteUnlit
{
    constexpr float minScreenSize = 0.025f;
    constexpr float maxScreenSize = 0.125f;

    struct SpriteProperties
    {
        Matrix MVP; // billboard * view * projection
        Color Color;
        float ScreenSize;
        float Padding[3];

        SpriteProperties();
    };

    struct SpriteUnlitVertex
    {
        Vector3 Position;
        Vector2 UV;
    };

    static const SpriteUnlitVertex QuadVertices[4] =
    {
        {Vector3(-1.0f, -1.0f, 0.0f), Vector2(0.0f, 0.0f)},
        {Vector3(+1.0f, -1.0f, 0.0f), Vector2(1.0f, 0.0f)},
        {Vector3(-1.0f, +1.0f, 0.0f), Vector2(0.0f, 1.0f)},
        {Vector3(+1.0f, +1.0f, 0.0f), Vector2(1.0f, 1.0f)},
    };

    static const uint16_t QuadIndices[6] =
    {
        0, 1, 2,
        2, 1, 3
    };

    static D3D12_INPUT_ELEMENT_DESC SpriteUnlitInputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    enum RootParameters
    {
        //// Shared shader parameter ////
        RootParameterSpriteProperties, // ConstantBuffer<SpriteProperties> SpritePropertiesCB : register(b0)

        //// Pixel shader parameters ////
        RootParameterTextures, // Texture2D SpriteTexture : register( t0 );

        RootParameterCount
    };

    bool SpriteUnlitShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);
    std::shared_ptr<Shader> GetSpriteUnlitShader(ComPtr<ID3D12Device2> device = nullptr);

    std::shared_ptr<Mesh> GetMeshForSpriteShape(std::shared_ptr<CommandList> commandList, SpriteShape spriteShape);
}
