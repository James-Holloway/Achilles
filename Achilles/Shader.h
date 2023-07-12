#pragma once
#include "Common.h"
#include <directx-dxc/dxcapi.h>
#include "CommandList.h"
#include "ShaderResourceView.h"

using Microsoft::WRL::ComPtr;

class Shader;
class Object;
class Mesh;
class Camera;
class Material;
class LightData;
struct aiScene;
struct aiMesh;

typedef void (CALLBACK* ShaderRender)(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object>, uint32_t knitIndex, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera, LightData& lightData);

typedef std::shared_ptr<Mesh> (CALLBACK* MeshCreation)(aiScene* scene, aiMesh* inMesh, std::shared_ptr<Shader> shader, Material& material);

HRESULT CompileShader(std::wstring shaderPath, std::wstring entry, std::wstring profile, ComPtr<IDxcResult>& outShader);

class Shader
{
public:
    std::wstring name;
    D3D12_INPUT_ELEMENT_DESC* vertexLayout;
    size_t vertexSize;
    // Root signature
    std::shared_ptr<RootSignature> rootSignature;
    // Pipeline state object.
    ComPtr<ID3D12PipelineState> pipelineState;
    // SRV that can be used to pad unused texture slots
    std::shared_ptr<ShaderResourceView> defaultSRV;

    ShaderRender renderCallback;
    MeshCreation meshCreateCallback;

    Shader(std::wstring _name, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, size_t _vertexSize, ShaderRender _renderCallback);

    // Binds a texture to the SRV for use in pixel shaders
    void BindTexture(CommandList& commandList, uint32_t rootParamIndex, uint32_t offset, std::shared_ptr<Texture>& texture);

    // Presumes shader file is contains two entrypoints - VS + PS for vertex and pixel shaders respectively
    static std::shared_ptr<Shader> ShaderVSPS(ComPtr<ID3D12Device2> device, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, UINT vertexLayoutCount, size_t _vertexSize, std::shared_ptr<RootSignature> rootSignature, ShaderRender _renderCallback, std::wstring shaderName);
};

inline void ThrowBlobIfFailed(HRESULT hr, ComPtr<ID3DBlob> errorBlob)
{
    if (SUCCEEDED(hr))
        return;
    
    if (errorBlob == nullptr)
        throw std::exception(); // if you hit here then you might need to ThrowBlobIfFailed on a line after storing the HRESULT rather than one-lining it

    OutputDebugStringA((char*)errorBlob->GetBufferPointer());

    throw std::exception();
}

#define SHADER_MESH_MAKE_SHARED_VECTORS(name, verts, tris, vertexStruct, shader) \
    std::make_shared<Mesh>(name, Object::GetCreationCommandList(), verts.data(), (UINT)verts.size(), sizeof(vertexStruct), tris.data(), (UINT)tris.size(), shader)