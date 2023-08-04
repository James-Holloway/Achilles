#pragma once

#include "../ShaderInclude.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace DebugWireframe
{
    struct Data
    {
        Matrix MVP; // Model * View * Projection, used for SV_Position
        Matrix Model;
        Color Color;
    };

    enum RootParameters
    {
        RootParameterData,
        RootParameterCount
    };

    bool DebugWireframeShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);
    std::shared_ptr<Shader> GetDebugWireframeShader(ComPtr<ID3D12Device2> device);
};

