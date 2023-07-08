#pragma once

#include "DescriptorAllocation.h"
#include <d3d12.h>
#include <memory>

class ConstantBuffer;

class ConstantBufferView
{
public:
    std::shared_ptr<ConstantBuffer> GetConstantBuffer() const
    {
        return constantBuffer;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle()
    {
        return descriptor.GetDescriptorHandle();
    }
    ConstantBufferView(const std::shared_ptr<ConstantBuffer>& _constantBuffer, size_t offset = 0);
    virtual ~ConstantBufferView() = default;

protected:
    std::shared_ptr<ConstantBuffer> constantBuffer;
    DescriptorAllocation            descriptor;
};
