#include "ShadowMapping.h"
#include "../LightObject.h"

using namespace ShadowMapping;
using namespace CommonShader;

ShadowMapping::ShadowMatrices::ShadowMatrices() : MVP()
{

}

static std::shared_ptr<Shader> ShadowMappingShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC ShadowMappingRootSignature{};
std::shared_ptr<Shader> ShadowMapping::GetShadowMappingShader(ComPtr<ID3D12Device2> device)
{
    if (ShadowMappingShader.use_count() >= 1)
        return ShadowMappingShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 textures = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset 0, space 0

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstants(sizeof(ShadowMatrices) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &textures, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    ShadowMappingRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags); // 1 static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(ShadowMappingRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    ShadowMappingShader = Shader::ShaderDepthOnlyVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, L"ShadowMapping", 500); // Use low bias for perspective lights

    return ShadowMappingShader;
}

static std::shared_ptr<Shader> ShadowMappingHighBiasShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC ShadowMappingHighBiasRootSignature{};
std::shared_ptr<Shader> ShadowMapping::GetShadowMappingHighBiasShader(ComPtr<ID3D12Device2> device)
{
    if (ShadowMappingHighBiasShader.use_count() >= 1)
        return ShadowMappingHighBiasShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 textures = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset 0, space 0

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterMatrices].InitAsConstants(sizeof(ShadowMatrices) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &textures, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    ShadowMappingHighBiasRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags); // 1 static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(ShadowMappingHighBiasRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    ShadowMappingHighBiasShader = Shader::ShaderDepthOnlyVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, L"ShadowMapping", 100000); // Use high bias for orthographic lights

    return ShadowMappingHighBiasShader;
}


void ShadowMapping::DrawObjectShadowDirectional(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, DirectionalLight directionalLight, std::shared_ptr<Shader> shader)
{
    ScopedTimer _prof(L"DrawObjectShadowDirectional");

    if (!object->ShouldDraw(shadowCamera->GetFrustum()))
        return;

    ShadowMapping::ShadowMatrices shadowMatrices{};
    shadowMatrices.MVP = (object->GetWorldMatrix() * (shadowCamera->GetView() * shadowCamera->GetProj()));
    commandList->SetGraphics32BitConstants<ShadowMapping::ShadowMatrices>(ShadowMapping::RootParameterMatrices, shadowMatrices);

    for (uint32_t i = 0; i < object->GetKnitCount(); i++)
    {
        Knit knit = object->GetKnit(i);
        std::shared_ptr<Mesh> mesh = knit.mesh;
        Material material = knit.material;
        commandList->SetMesh(mesh);

        std::shared_ptr<Texture> mainTexture = material.GetTexture(L"MainTexture");
        if (mainTexture == nullptr || !mainTexture->IsValid())
            mainTexture = Texture::GetCachedTexture(L"White");

        commandList->SetShaderResourceView(ShadowMapping::RootParameters::RootParameterTextures, 0, *mainTexture);

        commandList->DrawMesh(mesh);
    }
}

void ShadowMapping::DrawObjectShadowDirectionalCascaded(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, DirectionalLight directionalLight, std::shared_ptr<Shader> shader, Matrix cascadeMatrix)
{
    ScopedTimer _prof(L"DrawObjectShadowDirectionalCascaded");

    if (!object->ShouldDraw(shadowCamera->GetFrustum()))
        return;

    ShadowMapping::ShadowMatrices shadowMatrices{};
    shadowMatrices.MVP = object->GetWorldMatrix() * cascadeMatrix;
    commandList->SetGraphics32BitConstants<ShadowMapping::ShadowMatrices>(ShadowMapping::RootParameterMatrices, shadowMatrices);

    for (uint32_t i = 0; i < object->GetKnitCount(); i++)
    {
        Knit knit = object->GetKnit(i);
        std::shared_ptr<Mesh> mesh = knit.mesh;
        Material material = knit.material;
        commandList->SetMesh(mesh);

        std::shared_ptr<Texture> mainTexture = material.GetTexture(L"MainTexture");
        if (mainTexture == nullptr || !mainTexture->IsValid())
            mainTexture = Texture::GetCachedTexture(L"White");

        commandList->SetShaderResourceView(ShadowMapping::RootParameters::RootParameterTextures, 0, *mainTexture);

        commandList->DrawMesh(mesh);
    }
}

void ShadowMapping::DrawObjectShadowSpot(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, SpotLight spotLight, std::shared_ptr<Shader> shader)
{
    ScopedTimer _prof(L"DrawObjectShadowSpot");

    if (!object->ShouldDraw(shadowCamera->GetFrustum()))
        return;

    ShadowMapping::ShadowMatrices shadowMatrices{};
    shadowMatrices.MVP = (object->GetWorldMatrix() * (shadowCamera->GetView() * shadowCamera->GetProj()));
    commandList->SetGraphics32BitConstants<ShadowMapping::ShadowMatrices>(ShadowMapping::RootParameterMatrices, shadowMatrices);

    for (uint32_t i = 0; i < object->GetKnitCount(); i++)
    {
        Knit knit = object->GetKnit(i);
        std::shared_ptr<Mesh> mesh = knit.mesh;
        Material material = knit.material;
        commandList->SetMesh(mesh);

        std::shared_ptr<Texture> mainTexture = material.GetTexture(L"MainTexture");
        if (mainTexture == nullptr || !mainTexture->IsValid())
            mainTexture = Texture::GetCachedTexture(L"White");
        commandList->SetShaderResourceView(ShadowMapping::RootParameters::RootParameterTextures, 0, *mainTexture);

        commandList->DrawMesh(mesh);
    }
}

void ShadowMapping::DrawShadowDirectionalCascaded(std::shared_ptr<CommandList> commandList, std::vector<std::shared_ptr<Object>> shadowCastingObjects, std::shared_ptr<ShadowCamera> shadowCamera, LightObject* lightObject, DirectionalLight directionalLight, std::shared_ptr<Shader> shader)
{
    for (uint32_t cascade = 0; cascade < shadowCamera->GetNumCascades(); cascade++)
    {
        std::shared_ptr<ShadowMap> shadowMap = shadowCamera->GetShadowMap(cascade);
        LightObject* lightObject = shadowCamera->GetLightObject();

        commandList->TransitionBarrier(*shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        commandList->ClearDepthStencilTexture(*shadowMap, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0);
        std::shared_ptr<RenderTarget> rt = std::make_shared<RenderTarget>();
        rt->AttachTexture(AttachmentPoint::DepthStencil, shadowMap);
        commandList->SetRenderTargetDepthOnly(*rt);

        Matrix cascadeProj = shadowCamera->GetCascadeProjections()[cascade];
        Matrix cascadeMatrix = shadowCamera->GetView() * cascadeProj;

        for (std::shared_ptr<Object> object : shadowCastingObjects)
        {
            DrawObjectShadowDirectionalCascaded(commandList, object, shadowCamera, lightObject, directionalLight, shader, cascadeMatrix);
        }
    }
}

void ShadowMapping::RenderShadowScene(std::shared_ptr<CommandList> commandList, std::shared_ptr<ShadowCamera> shadowCamera, std::vector<std::shared_ptr<Object>> shadowCastingObjects)
{
    ScopedTimer _prof(L"RenderShadowScene");
    std::shared_ptr<RenderTarget> rt = shadowCamera->GetShadowMapRenderTarget();
    std::shared_ptr<ShadowMap> shadowMap = shadowCamera->GetShadowMap();
    rt->AttachTexture(AttachmentPoint::DepthStencil, shadowMap);

    LightObject* lightObject = shadowCamera->GetLightObject();

    if (lightObject == nullptr)
        return;

    if (shadowCamera->GetLightType() == LightType::Directional)
        commandList->SetShader(ShadowMappingHighBiasShader);
    else
        commandList->SetShader(ShadowMappingShader);

    commandList->SetScissorRect(shadowCamera->scissorRect);
    commandList->SetViewport(shadowCamera->viewport);

#pragma warning (suppress : 26813)
    if (shadowCamera->GetLightType() == LightType::Directional)
    {
        if (shadowCamera->GetNumCascades() <= 0)
        {
            commandList->TransitionBarrier(*shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            commandList->ClearDepthStencilTexture(*shadowMap, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0);
            commandList->SetRenderTargetDepthOnly(*rt);

            for (std::shared_ptr<Object> object : shadowCastingObjects)
            {
                DrawObjectShadowDirectional(commandList, object, shadowCamera, lightObject, lightObject->GetDirectionalLight(), ShadowMappingHighBiasShader);
            }
        }
        else
        {
            DrawShadowDirectionalCascaded(commandList, shadowCastingObjects, shadowCamera, lightObject, lightObject->GetDirectionalLight(), ShadowMappingHighBiasShader);
        }
    }
    else
    {
        commandList->TransitionBarrier(*shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ClearDepthStencilTexture(*shadowMap, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0);
        commandList->SetRenderTargetDepthOnly(*rt);

        if (shadowCamera->GetLightType() == LightType::Spot)
        {
            for (std::shared_ptr<Object> object : shadowCastingObjects)
            {
                DrawObjectShadowSpot(commandList, object, shadowCamera, lightObject, lightObject->GetSpotLight(), ShadowMappingShader);
            }
        }
        else if (shadowCamera->GetLightType() == LightType::Point)
        {
            /*for (std::shared_ptr<Object> object : shadowCastingObjects)
            {

            }*/
        }
    }
}
