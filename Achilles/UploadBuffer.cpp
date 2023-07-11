#include "UploadBuffer.h"
#include "Application.h"
#include "MathHelpers.h"

UploadBuffer::UploadBuffer(size_t _pageSize) : pageSize(_pageSize)
{

}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t _sizeInBytes, size_t _alignment)
{
    if (_sizeInBytes > pageSize)
    {
        throw std::bad_alloc();
    }

    // If there is no current age of the request allocation exceeds the remaining space in the current page, request a new page
    if (!currentPage || !currentPage->HasSpace(_sizeInBytes, _alignment))
    {
        currentPage = RequestPage();
    }

    return currentPage->Allocate(_sizeInBytes, _alignment);
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;
    if (!availablePages.empty()) // if we are not out of pages get the one at the front of the queue
    {
        page = availablePages.front();
        availablePages.pop_front();
    }
    else // if we are out of pages then create a new one
    {
        page = std::make_shared<Page>(pageSize);
        pagePool.push_back(page); // Put the new page at the back of the queue
    }

    return page;
}

// This can only happen when all the allocations are no longer in flight on the command queue
void UploadBuffer::Reset()
{
    currentPage = nullptr;
    // Reset all avaliable pages
    availablePages = pagePool;
    for (std::shared_ptr<Page> page : availablePages)
    {
        // Reset the page for new allocations
        page->Reset();
    }
}

UploadBuffer::Page::Page(size_t _sizeInBytes) : pageSize(_sizeInBytes), offset(0), CPUPtr(nullptr), GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    auto device = Application::GetD3D12Device();

    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(pageSize);
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&d3d12Resource)));

    GPUPtr = d3d12Resource->GetGPUVirtualAddress();
    d3d12Resource->Map(0, nullptr, &CPUPtr);
}

UploadBuffer::Page::~Page()
{
    d3d12Resource->Unmap(0, nullptr);
    CPUPtr = nullptr;
    GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::HasSpace(size_t _sizeInBytes, size_t _alignment) const
{
    size_t alignedSize = AlignUp(_sizeInBytes, _alignment);
    size_t alignedOffset = AlignUp(offset, _alignment);

    return alignedOffset + alignedSize <= pageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t _sizeInBytes, size_t alignment)
{
    if (!HasSpace(_sizeInBytes, alignment))
    {
        throw std::bad_alloc(); // Can't allocate space from page.
    }

    std::lock_guard<std::mutex> lock(pageMutex); // should stop the page from allocating at the same time on different threads

    size_t alignedSize = AlignUp(_sizeInBytes, alignment);
    offset = AlignUp(offset, alignment);

    Allocation allocation{};
    allocation.CPU = static_cast<uint8_t*>(CPUPtr) + offset;
    allocation.GPU = GPUPtr + offset;

    offset += alignedSize;

    return allocation;
}