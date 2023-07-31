#pragma once

#include "../ShaderInclude.h"
#include "CommonShader.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Color;

namespace Skybox
{
    struct SkyboxInfo
    {
        Matrix MVP;
        Matrix Model;

        float HasTexture;
        Vector3 LookDirection;

        float PrimarySunEnable;
        Vector3 PrimarySunDirection;

        Color PrimarySunColor;

        float PrimarySunSize;
        float PrimarySunShineExponent;
        float HideSunBehindHorizon;
        float Debug;

        Color SkyColor;
        Color UpSkyColor;
        Color HorizonColor;
        Color GroundColor;

        SkyboxInfo();
    };

    enum RootParameters
    {
        // Pixel Shader Parameters
        RootParameterSkyboxInfo,
        RootParameterCubemap,

        RootParameterCount
    };

    bool SkyboxShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);
    std::shared_ptr<Mesh> SkyboxMeshCreation(aiScene* scene, aiNode* node, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material);
    std::shared_ptr<Shader> GetSkyboxShader(ComPtr<ID3D12Device2> device = nullptr);
}
