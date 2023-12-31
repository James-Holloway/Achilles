#pragma once

#include "Common.h"
#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
    VertexBuffer(const std::wstring& _name = L"");
    virtual ~VertexBuffer();

    // Inherited from Buffer
    virtual void CreateViews(size_t _numElements, size_t _elementSize) override;

    // Get the vertex buffer view for binding to the Input Assembler stage.
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
    {
        return vertexBufferView;
    }

    size_t GetNumVertices() const
    {
        return numVertices;
    }

    size_t GetVertexStride() const
    {
        return vertexStride;
    }

    // Get the SRV for a resource
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

    // Get the UAV for a (sub)resource
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

protected:

private:
    size_t numVertices;
    size_t vertexStride;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};
