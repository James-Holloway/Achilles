#include "DescriptorAllocator.h"
#include "DescriptorAllocatorPage.h"

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorsPerHeap) : heapType(_type), numDescriptorsPerHeap(_numDescriptorsPerHeap) {}

DescriptorAllocator::~DescriptorAllocator() {}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
    auto newPage = std::make_shared<DescriptorAllocatorPage>(heapType, numDescriptorsPerHeap);

    heapPool.emplace_back(newPage);
    availableHeaps.insert(heapPool.size() - 1);

    return newPage;
}

DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(allocationMutex);

    DescriptorAllocation allocation;

    for (auto iter = availableHeaps.begin(); iter != availableHeaps.end(); ++iter)
    {
        std::shared_ptr<DescriptorAllocatorPage> allocatorPage = heapPool[*iter];

        allocation = allocatorPage->Allocate(numDescriptors);

        if (allocatorPage->NumFreeHandles() == 0)
        {
            iter = availableHeaps.erase(iter);
        }

        // A valid allocation has been found.
        if (!allocation.IsNull())
        {
            break;
        }
    }

    // No available heap could satisfy the requested number of descriptors.
    if (allocation.IsNull())
    {
        numDescriptorsPerHeap = std::max(numDescriptorsPerHeap, numDescriptors);
        std::shared_ptr<DescriptorAllocatorPage> newPage = CreateAllocatorPage();

        allocation = newPage->Allocate(numDescriptors);
    }

    return allocation;
}

void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(allocationMutex);

    for (size_t i = 0; i < heapPool.size(); ++i)
    {
        std::shared_ptr<DescriptorAllocatorPage> page = heapPool[i];

        page->ReleaseStaleDescriptors(frameNumber);

        if (page->NumFreeHandles() > 0)
        {
            availableHeaps.insert(i);
        }
    }
}