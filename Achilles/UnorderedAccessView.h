#pragma once

#include "DescriptorAllocation.h"
#include "Resource.h"
#include <d3d12.h>
#include <memory>

class UnorderedAccessView
{
public:
    std::shared_ptr<Resource> GetResource() const
    {
        return resource;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const
    {
        return descriptor.GetDescriptorHandle();
    }

    UnorderedAccessView(const std::shared_ptr<Resource>& _resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr);
    virtual ~UnorderedAccessView() = default;

protected:
    std::shared_ptr<Resource> resource;
    DescriptorAllocation descriptor;
};
