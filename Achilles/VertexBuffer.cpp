#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(const std::wstring& _name) : Buffer(_name), numVertices(0), vertexStride(0), vertexBufferView({}) {}

VertexBuffer::~VertexBuffer() {}

void VertexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
    numVertices = numElements;
    vertexStride = elementSize;

    vertexBufferView.BufferLocation = d3d12Resource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = static_cast<UINT>(numVertices * vertexStride);
    vertexBufferView.StrideInBytes = static_cast<UINT>(vertexStride);
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    throw std::exception("VertexBuffer::GetShaderResourceView should not be called.");
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    throw std::exception("VertexBuffer::GetUnorderedAccessView should not be called.");
}