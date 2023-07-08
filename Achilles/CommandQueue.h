#pragma once
#include "Common.h"
#include "ThreadSafeQueue.h"

using Microsoft::WRL::ComPtr;

class CommandList;

class CommandQueue
{
public:
	CommandQueue() = delete;
	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	// Get an available command list from the command queue.
	std::shared_ptr<CommandList> GetCommandList();

	// Execute a command list.
	// Returns the fence value to wait for for this command list.
	uint64_t ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	uint64_t ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList> >& commandLists);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

	// Wait for another command queue to finish.
	void Wait(const CommandQueue& other);

	ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

private:
	// Free any command lists that are finished processing on the command queue.
	void ProccessInFlightCommandLists();

	// Keep track of command allocators that are "in-flight"
	// The first member is the fence value to wait for, the second is the a shared pointer to the "in-flight" command list.
	using CommandListEntry = std::tuple<uint64_t, std::shared_ptr<CommandList>>;

	D3D12_COMMAND_LIST_TYPE commandListType;
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
	ComPtr<ID3D12Fence> d3d12Fence;
	std::atomic_uint64_t fenceValue;

	size_t commandListCreatedCount = 0;

	ThreadSafeQueue<CommandListEntry> inFlightCommandLists;
	ThreadSafeQueue<std::shared_ptr<CommandList>> availableCommandLists;

	// A thread to process in-flight command lists.
	std::thread processInFlightCommandListsThread;
	std::atomic_bool processInFlightCommandLists;
	std::mutex processInFlightCommandListsThreadMutex;
	std::condition_variable processInFlightCommandListsThreadCV;
};
