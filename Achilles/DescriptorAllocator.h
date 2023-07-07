#pragma once

#include <wrl.h>
#include <set>
#include <vector>
#include <mutex>
#include <cstdint>
#include "DescriptorAllocation.h"

using Microsoft::WRL::ComPtr;

class DescriptorAllocatorPage;

class DescriptorAllocator
{
public:
    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorsPerHeap = 256);
    virtual ~DescriptorAllocator();

    // Allocate a number of contiguous descriptors from a CPU visible descriptor heap
    // @param numDescriptors The number of contiguous descriptors to allocate (cannot be more than the number of descriptors per descriptor heap)
    DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

    // When the frame has completed, the stale descriptors can be released
    void ReleaseStaleDescriptors(uint64_t frameNumber);

private:
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    // Create a new heap with a specific number of descriptors.
    std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

    D3D12_DESCRIPTOR_HEAP_TYPE heapType;
    uint32_t numDescriptorsPerHeap;

    DescriptorHeapPool heapPool;
    // Indices of available heaps in the heap pool.
    std::set<size_t> availableHeaps;

    std::mutex allocationMutex;

    ComPtr<ID3D12Device2> device;
};