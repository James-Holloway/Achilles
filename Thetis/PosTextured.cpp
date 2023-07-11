#include "PosTextured.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace PosTextured
{
    void PosTexturedShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera)
    {
        // Update the MVP matrix
        Matrix mvp = object->GetWorldMatrix() * (camera->GetView() * camera->GetProj());

        PosTexturedCB0 cb0{ mvp };

        std::shared_ptr<Texture> mainTexture = nullptr;

        auto mainTextureIter = material.textures.find(L"MainTexture");
        if (mainTextureIter != material.textures.end())
            mainTexture = mainTextureIter->second;

        material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, mainTexture);

        commandList->SetGraphics32BitConstants<PosTexturedCB0>(0, cb0);
    }

    static std::shared_ptr<Shader> posTexturedShader{};
    static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC polTexturedRootSignature{};
    std::shared_ptr<Shader> GetPosTexturedShader(ComPtr<ID3D12Device2> device)
    {
        if (posTexturedShader.use_count() >= 1)
        {
            return posTexturedShader;
        }

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        // Textures descriptor range
        CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset at 0, in space 0

        // Root parameters
        CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
        rootParameters[RootParameters::RootParameterPosTexturedCB0].InitAsConstants(sizeof(PosTexturedCB0) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

        // Sampler(s)
        CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // anisotropic sampler set to 8

        polTexturedRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags); // 1 is number of static samplers

        std::shared_ptr rootSignature = std::make_shared<RootSignature>(polTexturedRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

        posTexturedShader = Shader::ShaderVSPS(device, posTexturedInputLayout, _countof(posTexturedInputLayout), sizeof(PosTexturedVertex), rootSignature, PosTexturedShaderRender, L"PosTextured");

        return posTexturedShader;
    }
}