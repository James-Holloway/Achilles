#pragma once

#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
    IndexBuffer(const std::wstring& _name = L"");
    virtual ~IndexBuffer();

    // Inherited from Buffer
    virtual void CreateViews(size_t _numElements, size_t _elementSize) override;

    size_t GetNumIndices() const
    {
        return numIndices;
    }

    DXGI_FORMAT GetIndexFormat() const
    {
        return indexFormat;
    }

    size_t GetIndexStride() const
    {
        return indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    }

    // Get the index buffer view for biding to the Input Assembler stage.
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
    {
        return indexBufferView;
    }

    // Get the SRV for a resource.
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

    // Get the UAV for a (sub)resource.
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

protected:

private:
    size_t numIndices;
    DXGI_FORMAT indexFormat;

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
};