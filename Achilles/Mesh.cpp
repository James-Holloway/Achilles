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

std::shared_ptr<VertexBuffer> Mesh::GetVertexBuffer()
{
    return vertexBuffer;
}

std::shared_ptr<IndexBuffer> Mesh::GetIndexBuffer()
{
    return indexBuffer;
}

std::vector<Vector3> Mesh::GetTrianglePoints(Matrix transformMatrix)
{
    std::vector<Vector3> outPositions;
    std::shared_ptr<VertexBuffer> vbuffer = GetVertexBuffer();
    std::shared_ptr<IndexBuffer> ibuffer = GetIndexBuffer();

    if (vbuffer == nullptr || ibuffer == nullptr)
        return outPositions;

    size_t vbufferSize = vbuffer->GetBufferSize();
    size_t vertexStride = vbuffer->GetVertexStride();

    size_t indexCount = ibuffer->GetNumIndices();
    size_t indexStride = ibuffer->GetIndexStride();

    char* positions = reinterpret_cast<char*>(vbuffer->GetBufferData());
    void* indices = ibuffer->GetBufferData();

    for (size_t i = 0; i < indexCount * indexStride; i += indexStride)
    {
        uint32_t index = 0;
        if (indexStride == 2)
            index = (uint32_t)(reinterpret_cast<uint16_t*>(indices)[i / indexStride]);
        else if (indexStride == 4)
            index = reinterpret_cast<uint32_t*>(indices)[i / indexStride];
        else
            return outPositions;

        if (index * vertexStride >= vbufferSize)
            continue;

        // Get position from offset by index * stride
        Vector3 position = *(reinterpret_cast<Vector3*>(positions + (index * vertexStride)));
        position = Multiply(transformMatrix, position);

        outPositions.push_back(position);
    }

    return outPositions;
}

bool Mesh::HasBeenCopied()
{
    if (vertexBuffer == nullptr || indexBuffer == nullptr)
        return false;
    return vertexBuffer->HasBeenCopied() && indexBuffer->HasBeenCopied();
}
