#include "Mesh.h"
#include "ShaderInclude.h"
#include "Application.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Mesh::Mesh(std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader) : shader(_shader)
{
    topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    vertexBuffer = std::make_shared<VertexBuffer>(L"Unnamed Mesh Vertex Buffer");
    indexBuffer = std::make_shared<IndexBuffer>(L"Unnamed Mesh Index Buffer");

    commandList->CopyVertexBuffer(*vertexBuffer, vertexCount, vertexStride, vertices);
    commandList->CopyIndexBuffer(*indexBuffer, indexCount, DXGI_FORMAT_R16_UINT, indices);

    isCreated = true;
}

Mesh::Mesh(std::wstring _name, std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader)
    : Mesh(commandList, vertices, vertexCount, vertexStride, indices, indexCount, _shader)
{
    SetName(_name);
}

void Mesh::SetName(std::wstring newName)
{
    name = newName;
    if (vertexBuffer)
        vertexBuffer->SetName(name + L" Vertex Buffer");
    if (indexBuffer)
        indexBuffer->SetName(name + L" Index Buffer");
}

DirectX::BoundingBox Mesh::GetBoundingBox()
{
    return boundingBox;
}

void Mesh::SetBoundingBox(DirectX::BoundingBox box)
{
    boundingBox = box;
}

bool Mesh::HasBeenCopied()
{
    if (vertexBuffer == nullptr || indexBuffer == nullptr)
        return false;
    return vertexBuffer->HasBeenCopied() && indexBuffer->HasBeenCopied();
}
