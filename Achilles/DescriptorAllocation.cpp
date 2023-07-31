#include "DescriptorAllocation.h"
#include "DescriptorAllocatorPage.h"
#include "Application.h"
#include "Profiling.h"

DescriptorAllocation::DescriptorAllocation() : descriptor{ 0 }, numHandles(0), descriptorSize(0), page(nullptr)
{

}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE _descriptor, uint32_t _numHandles, uint32_t _descriptorSize, std::shared_ptr<DescriptorAllocatorPage> _page)
    : descriptor(_descriptor), numHandles(_numHandles), descriptorSize(_descriptorSize), page(_page)
{

}

DescriptorAllocation::~DescriptorAllocation()
{
    Free();
}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation) noexcept : descriptor(allocation.descriptor), numHandles(allocation.numHandles), descriptorSize(allocation.descriptorSize), page(std::move(allocation.page))
{
    // Invalidate the original
    allocation.descriptor.ptr = 0;
    allocation.numHandles = 0;
    allocation.descriptorSize = 0;
}

DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other) noexcept
{
    // Free this descriptor if it points to anything.
    Free();

    // Copy from other
    descriptor = other.descriptor;
    numHandles = other.numHandles;
    descriptorSize = other.descriptorSize;
    page = std::move(other.page);

    // Invalidate other
    other.descriptor.ptr = 0;
    other.numHandles = 0;
    other.descriptorSize = 0;

    return *this;
}

void DescriptorAllocation::Free()
{
    if (!IsNull() && page)
    {
        // Don't add the block directly to the free list until the frame has completed
        page->Free(std::move(*this), Application::GetGlobalFrameCounter());

        descriptor.ptr = 0;
        numHandles = 0;
        descriptorSize = 0;
        page.reset();
    }
}

// Check if this a valid descriptor
bool DescriptorAllocation::IsNull() const
{
    return descriptor.ptr == 0;
}

// Get a descriptor at a particular offset in the allocation
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t offset) const
{
    assert(offset < numHandles);
    return { descriptor.ptr + (descriptorSize * offset) };
}

uint32_t DescriptorAllocation::GetNumHandles() const
{
    return numHandles;
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage() const
{
    return page;
}