#pragma once

#include "Common.h"
#include "Buffer.h"
#include "ByteAddressBuffer.h"

class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(const std::wstring& name = L"");
    StructuredBuffer(const D3D12_RESOURCE_DESC& resDesc, size_t _numElements, size_t _elementSize, const std::wstring& name = L"");

    /**
    * Get the number of elements contained in this buffer.
    */
    virtual size_t GetNumElements() const
    {
        return numElements;
    }

    /**
    * Get the size in bytes of each element in this buffer.
    */
    virtual size_t GetElementSize() const
    {
        return elementSize;
    }

    /**
     * Create the views for the buffer resource.
     * Used by the CommandList when setting the buffer contents.
     */
    virtual void CreateViews(size_t numElements, size_t elementSize) override;

    /**
     * Get the SRV for a resource.
     */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const
    {
        return SRV.GetDescriptorHandle();
    }

    /**
     * Get the UAV for a (sub)resource.
     */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override
    {
        // Buffers don't have subresources.
        return UAV.GetDescriptorHandle();
    }

    const ByteAddressBuffer& GetCounterBuffer() const
    {
        return counterBuffer;
    }

private:
    size_t numElements;
    size_t elementSize;

    DescriptorAllocation SRV;
    DescriptorAllocation UAV;

    // A buffer to store the internal counter for the structured buffer.
    ByteAddressBuffer counterBuffer;
};