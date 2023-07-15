#include "SpriteUnlit.h"
#include "../SpriteObject.h"
#include "../Application.h"

using namespace SpriteUnlit;
using namespace CommonShader;

SpriteUnlit::SpriteProperties::SpriteProperties() : MVP(), Color(0, 0, 0, 0), ScreenSize(0.025f), Padding{0}
{

}

bool SpriteUnlit::SpriteUnlitShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData)
{
    // If this isn't a sprite object then don't render
    if (!object->HasTag(ObjectTag::Sprite))
        return false;

    std::shared_ptr<SpriteObject> spriteObject = std::dynamic_pointer_cast<SpriteObject>(object);

    Vector3 position = spriteObject->GetWorldPosition();

    SpriteProperties spriteProperties{};
    spriteProperties.Color = spriteObject->GetSpriteColor();

    bool visible = false;
    Vector3 screenPos = camera->WorldToScreen(position, visible);
    if (!visible)
        return false;

    Matrix billboard = camera->GetBillboardMatrix(position);
    Matrix mvp = (billboard * camera->GetView()) * camera->GetProj();
    spriteProperties.MVP = mvp;

    float distance = (camera->GetPosition() - position).Length();
    float size = std::lerp(minScreenSize, maxScreenSize, std::clamp<float>(distance, 0.0f, 1.0f));
    spriteProperties.ScreenSize = size;

    commandList->SetGraphicsDynamicConstantBuffer<SpriteProperties>(RootParameters::RootParameterSpriteProperties, spriteProperties);

    std::shared_ptr<Texture> spriteTexture = spriteObject->GetSpriteTexture();
    material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, spriteTexture);

    return true;
}

static std::shared_ptr<Shader> SpriteUnlitShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC SpriteUnlitRootSignature{};
std::shared_ptr<Shader> SpriteUnlit::GetSpriteUnlitShader(ComPtr<ID3D12Device2> device)
{
    if (SpriteUnlitShader.use_count() >= 1)
        return SpriteUnlitShader;

    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

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
    rootParameters[RootParameters::RootParameterSpriteProperties].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);

    rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // Sampler(s)
    CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // anisotropic sampler set to 8

    // Root signature creation
    SpriteUnlitRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags); // 1 is number of static samplers
    std::shared_ptr rootSignature = std::make_shared<RootSignature>(SpriteUnlitRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    SpriteUnlitShader = Shader::ShaderVSPS(device, SpriteUnlitInputLayout, _countof(SpriteUnlitInputLayout), sizeof(CommonShaderVertex), rootSignature, SpriteUnlitShaderRender, L"SpriteUnlit", D3D12_CULL_MODE_FRONT, true);

    return SpriteUnlitShader;
}

static bool hasConstructedMeshes = false;
static std::shared_ptr<Mesh> spriteShapeSquareMesh = nullptr;

static void ConstructMeshes(std::shared_ptr<CommandList> commandList)
{
    if (hasConstructedMeshes)
        return;

    auto device = Application::GetD3D12Device();
    std::shared_ptr<Shader> shader = GetSpriteUnlitShader(device);

    // std::wstring _name, std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader
    spriteShapeSquareMesh = std::make_shared<Mesh>(L"SpriteUnlit Square Mesh", commandList, (void*) QuadVertices, (UINT)_countof(QuadVertices), sizeof(SpriteUnlitVertex), QuadIndices, (UINT)_countof(QuadIndices), shader);

    hasConstructedMeshes = true;
}

std::shared_ptr<Mesh> SpriteUnlit::GetMeshForSpriteShape(std::shared_ptr<CommandList> commandList, SpriteShape spriteShape)
{
    ConstructMeshes(commandList);
    switch (spriteShape)
    {
    case SpriteShape::Square:
        return spriteShapeSquareMesh;
    }
    return nullptr;
}
