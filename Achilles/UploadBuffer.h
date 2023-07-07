#pragma once

#include "Common.h"
#include <new>

// https://www.3dgep.com/learning-directx-12-3/#UploadBuffer_Class

using Microsoft::WRL::ComPtr;

class UploadBuffer
{
public:
	// Use to upload data to the GPU
	struct Allocation
	{
		void* CPU;
		D3D12_GPU_VIRTUAL_ADDRESS GPU;
	};

	// pageSize is The size to use to allocate new pages in GPU memory
	explicit UploadBuffer(size_t _pageSize = _2MB);

	// The maximum size of an allocation is the size of a single page
	size_t GetPageSize() const { return pageSize; }

	// Allocate memory in an Upload heap. An allocation must not exceed the size of a page
	// Use a memcpy or similar method to copy the buffer data to CPU pointer in the Allocation structure returned from this function
	Allocation Allocate(size_t _sizeInBytes, size_t _alignment);

	//	Release all allocated pages. This should only be done when the command list is finished executing on the CommandQueue
	void Reset();

private:
	// A single page for the allocator
	struct Page
	{
		Page(size_t _sizeInBytes);
		~Page();

		// Check to see if the page has room to satisfy the requested allocation
		bool HasSpace(size_t _sizeInBytes, size_t _alignment) const;

		// Allocate memory from the page
		// Throws std::bad_alloc if the the allocation size is larger that the page size or the size of the allocation exceeds the remaining space in the page
		Allocation Allocate(size_t _sizeInBytes, size_t _alignment);

		// Reset the page for reuse (resets offset of page to 0)
		inline void Reset()
		{
			offset = 0;
		};

	private:
		ComPtr<ID3D12Resource> d3d12Resource;

		void* CPUPtr; // Base pointer
		D3D12_GPU_VIRTUAL_ADDRESS GPUPtr; // Contains the GPU memory for the page

		size_t pageSize; // Allocated page size
		size_t offset; // Current allocation offset in bytes

		std::mutex pageMutex;
	};

	// A pool of memory pages
	using PagePool = std::deque<std::shared_ptr<Page>>;

	// Request a page from the pool of available pages or create a new page if there are no available pages
	std::shared_ptr<Page> RequestPage();

	PagePool pagePool; // Used/created pages
	PagePool availablePages; // Pages that are available for allocation

	std::shared_ptr<Page> currentPage;

	// The size of each page of memory
	size_t pageSize;
};

