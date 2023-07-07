#include "DescriptorAllocatorPage.h"
#include "Application.h"

DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) : heapType(type), numDescriptorsInHeap(numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = heapType;
    heapDesc.NumDescriptors = numDescriptorsInHeap;

    auto device = Application::GetD3D12Device();

    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&d3d12DescriptorHeap)));

    baseDescriptor = d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(heapType);
    numFreeHandles = numDescriptorsInHeap;

    // Initialize the free lists
    AddNewBlock(0, numFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
    return heapType;
}

uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
    return numFreeHandles;
}

bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
    // lower_bounds finds first entry in the free list that is greater than or equal to numDescriptors
    return freeListBySize.lower_bound(numDescriptors) != freeListBySize.end();
}

void DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
    auto offsetIter = freeListByOffset.emplace(offset, numDescriptors);
    auto sizeIter = freeListBySize.emplace(numDescriptors, offsetIter.first);
    offsetIter.first->second.FreeListBySizeIt = sizeIter;
}

DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(allocationMutex);

    // There are less than the requested number of descriptors left in the heap. Return a NULL descriptor and try another heap.
    if (numDescriptors > numFreeHandles)
    {
        return DescriptorAllocation();
    }

    // Get the first block that is large enough to satisfy the request.
    auto smallestBlockIter = freeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIter == freeListBySize.end())
    {
        // There was no free block that could satisfy the request.
        return DescriptorAllocation();
    }

    // The size of the smallest block that satisfies the request.
    auto blockSize = smallestBlockIter->first;

    // The pointer to the same entry in the FreeListByOffset map.
    auto offsetIter = smallestBlockIter->second;

    // The offset in the descriptor heap.
    auto offset = offsetIter->first;

    // Remove the existing free block from the free list.
    freeListBySize.erase(smallestBlockIter);
    freeListByOffset.erase(offsetIter);

    // Compute the new free block that results from splitting this block.
    auto newOffset = offset + numDescriptors;
    auto newSize = blockSize - numDescriptors;

    if (newSize > 0)
    {
        // If the allocation didn't exactly match the requested size,
        // return the left-over to the free list.
        AddNewBlock(newOffset, newSize);
    }

    // Decrement free handles.
    numFreeHandles -= numDescriptors;

    return DescriptorAllocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(baseDescriptor, offset, descriptorHandleIncrementSize), numDescriptors, descriptorHandleIncrementSize, shared_from_this());
}

uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return static_cast<uint32_t>(handle.ptr - baseDescriptor.ptr) / descriptorHandleIncrementSize;
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptor, uint64_t frameNumber)
{
    // Compute the offset of the descriptor within the descriptor heap
    auto offset = ComputeOffset(descriptor.GetDescriptorHandle());

    std::lock_guard<std::mutex> lock(allocationMutex);

    // Don't add the block directly to the free list until the frame has completed
    staleDescriptors.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
    // Find the first element whose offset is greater than the specified offset. This is the block that should appear after the block that is being freed
    auto nextBlockIt = freeListByOffset.upper_bound(offset);

    // Find the block that appears before the block being freed
    auto prevBlockIter = nextBlockIt;
    // If it's not the first block in the list
    if (prevBlockIter != freeListByOffset.begin())
    {
        // Go to the previous block in the list.
        --prevBlockIter;
    }
    else
    {
        // Otherwise, just set it to the end of the list to indicate that no
        // block comes before the one being freed.
        prevBlockIter = freeListByOffset.end();
    }

    // Add the number of free handles back to the heap.
    // This needs to be done before merging any blocks since merging
    // blocks modifies the numDescriptors variable.
    numFreeHandles += numDescriptors;

    //  Case 1: There is a block in the free list that is immediately preceding the block being freed.
    if (prevBlockIter != freeListByOffset.end() && offset == prevBlockIter->first + prevBlockIter->second.Size)
    {
        // The previous block is exactly behind the block that is to be freed.
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //

        // Increase the block size by the size of merging with the previous block.
        offset = prevBlockIter->first;
        numDescriptors += prevBlockIter->second.Size;

        // Remove the previous block from the free list.
        freeListBySize.erase(prevBlockIter->second.FreeListBySizeIt);
        freeListByOffset.erase(prevBlockIter);
    }

    // Case 2: There is a block in the free list that is immediately following the block being freed
    if (nextBlockIt != freeListByOffset.end() && offset + numDescriptors == nextBlockIt->first)
    {
        // The next block is exactly in front of the block that is to be freed.
        //
        // Offset               NextBlock.Offset 
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|

        // Increase the block size by the size of merging with the next block.
        numDescriptors += nextBlockIt->second.Size;

        // Remove the next block from the free list.
        freeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
        freeListByOffset.erase(nextBlockIt);
    }

    // Add the freed block to the free list.
    AddNewBlock(offset, numDescriptors);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(allocationMutex);

    // Release stale descriptors. A descriptor is stale when the frame number is before the current
    while (!staleDescriptors.empty() && staleDescriptors.front().FrameNumber <= frameNumber)
    {
        auto& staleDescriptor = staleDescriptors.front();

        // The offset of the descriptor in the heap.
        auto offset = staleDescriptor.Offset;
        // The number of descriptors that were allocated.
        auto numDescriptors = staleDescriptor.Size;

        FreeBlock(offset, numDescriptors);

        staleDescriptors.pop();
    }
}