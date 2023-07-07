#pragma once

#include "Common.h"
#include "Buffer.h"
#include "DescriptorAllocation.h"

class ByteAddressBuffer : public Buffer
{
public:
    ByteAddressBuffer(const std::wstring& name = L"");
    ByteAddressBuffer(const D3D12_RESOURCE_DESC& resDesc,
        size_t numElements, size_t elementSize,
        const std::wstring& name = L"");

    size_t GetBufferSize() const
    {
        return bufferSize;
    }

    // Create the views for the buffer resource
    // Used by the CommandList when setting the buffer contents
    virtual void CreateViews(size_t numElements, size_t elementSize) override;

    // Get the SRV for a resource.
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override
    {
        return SRV.GetDescriptorHandle();
    }

    /**
     * Get the UAV for a (sub)resource.
     */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override
    {
        // Buffers only have a single subresource.
        return UAV.GetDescriptorHandle();
    }

protected:

private:
    size_t bufferSize;

    DescriptorAllocation SRV;
    DescriptorAllocation UAV;
};