#pragma once

#include <cstdint>
#include <cassert>
#include <d3dx12.h>
#include <wrl.h>
#include "Helpers.h"

using Microsoft::WRL::ComPtr;

class DescriptorAllocatorPage;

// Used to represent a single allocation from the descriptor heap.
class DescriptorAllocation
{
public:
    // Creates a NULL descriptor
    DescriptorAllocation();

    DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE _descriptor, uint32_t _numHandles, uint32_t _descriptorSize, std::shared_ptr<DescriptorAllocatorPage> _page);

    // The destructor will automatically free the allocation
    ~DescriptorAllocation();

    // Copies are not allowed
    DescriptorAllocation(const DescriptorAllocation&) = delete;
    DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

    // Move is allowed
    DescriptorAllocation(DescriptorAllocation&& allocation) noexcept;
    DescriptorAllocation& operator=(DescriptorAllocation&& other) noexcept;

    // Check if this a valid descriptor
    bool IsNull() const;

    // Get a descriptor at a particular offset in the allocation
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

    // Get the number of (consecutive) handles for this allocation.
    uint32_t GetNumHandles() const;

    // Get the heap that this allocation came from (for internal use only)
    std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;

private:
    // Free the descriptor back to the heap it came from.
    void Free();

    // The base descriptor.
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
    // The number of descriptors in this allocation.
    uint32_t numHandles;
    // The offset to the next descriptor.
    uint32_t descriptorSize;

    // A pointer back to the original page where this allocation came from.
    std::shared_ptr<DescriptorAllocatorPage> page;
};