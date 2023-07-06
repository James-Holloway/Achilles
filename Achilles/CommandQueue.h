#pragma once
#include "Common.h"
using Microsoft::WRL::ComPtr;

class CommandQueue
{
public:
	CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	// Get an available command list from the command queue.
	ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

	// Execute a command list.
	// Returns the fence value to wait for for this command list.
	uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

	ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

protected:
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);

private:
	// Keep track of command allocators that are "in-flight"
	struct CommandAllocatorEntry
	{
		uint64_t fenceValue;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	D3D12_COMMAND_LIST_TYPE commandListType;
	ComPtr<ID3D12Device2> d3d12Device;
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
	ComPtr<ID3D12Fence> d3d12Fence;
	HANDLE  fenceEvent;
	uint64_t fenceValue;

	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue = std::queue<ComPtr<ID3D12GraphicsCommandList2>>;

	CommandAllocatorQueue commandAllocatorQueue;
	CommandListQueue commandListQueue;
};
