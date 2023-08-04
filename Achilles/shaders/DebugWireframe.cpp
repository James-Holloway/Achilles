#include "DebugWireframe.h"
#include "CommonShader.h"

using namespace DebugWireframe;
using namespace CommonShader;

bool DebugWireframe::DebugWireframeShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData)
{
    Data data;
    data.Model = object->GetWorldMatrix();
    data.MVP = (data.Model * camera->GetView()) * camera->GetProj();
    data.Color = material.GetVector(L"Color");

    commandList->SetGraphics32BitConstants<Data>(RootParameters::RootParameterData, data);

    return true;
}

static std::shared_ptr<Shader> DebugWireframeShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC DebugWireframeRootSignature{};
std::shared_ptr<Shader> DebugWireframe::GetDebugWireframeShader(ComPtr<ID3D12Device2> device)
{
    if (DebugWireframeShader.use_count() >= 1)
    {
        return DebugWireframeShader;
    }
    if (device == nullptr)
        throw std::exception("Cannot create a shader without a device");

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    // Root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
    rootParameters[RootParameters::RootParameterData].InitAsConstants(sizeof(Data) / 4, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

    DebugWireframeRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags); // 1 is number of static samplers

    std::shared_ptr rootSignature = std::make_shared<RootSignature>(DebugWireframeRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

    DebugWireframeShader = Shader::ShaderWireframeVSPS(device, CommonShaderInputLayout, _countof(CommonShaderInputLayout), sizeof(CommonShaderVertex), rootSignature, DebugWireframeShaderRender, L"DebugWireframe");
    DebugWireframeShader->meshCreateCallback = CommonShaderMeshCreation;

    return DebugWireframeShader;
}
