#include "stdafx.h"
#include "CommandQueue.h"

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) : fenceValue(0), commandListType(type), d3d12Device(device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
	ThrowIfFailed(d3d12Device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12Fence)));

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL); // https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventw
	assert(fenceEvent && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{

}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> commandList;

	// Reuse current command allocator. Only does so if it has completed
	if (!commandAllocatorQueue.empty() && IsFenceComplete(commandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = commandAllocatorQueue.front().commandAllocator;
		commandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());
	}
	else
	{
		// Create new command allocator
		commandAllocator = CreateCommandAllocator();
	}

	if (!commandListQueue.empty())
	{
		commandList = commandListQueue.front();
		commandListQueue.pop();
		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	// Associate the command allocator with the command list so that it can be retrieved when the command list is executed.
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

// Executes a command list
// Returns the fence value used to wait for this command list
uint64_t CommandQueue::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	commandList->Close();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	// Retrieving a COM pointer of a COM object associated with the private data of the ID3D12Object object will also increment the reference counter of that COM object
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const ppCommandLists[] = {
		commandList.Get()
	};

	d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	commandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	commandListQueue.push(commandList);

	// The ownership of the command allocator has been transferred to the ComPtr in the command allocator queue. It is safe to release the reference in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

uint64_t CommandQueue::Signal()
{
	uint64_t fv = ++fenceValue;
	d3d12CommandQueue->Signal(d3d12Fence.Get(), fv);
	return fv;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	return d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
	if (!IsFenceComplete(fenceValue))
	{
		d3d12Fence->SetEventOnCompletion(fenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, DWORD_MAX);
	}
}

void CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
{
	return d3d12CommandQueue;
}

ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(d3d12Device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator)
{
	ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(d3d12Device->CreateCommandList(0, commandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}

