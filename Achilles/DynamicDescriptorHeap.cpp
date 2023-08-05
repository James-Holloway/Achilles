#include "DynamicDescriptorHeap.h"
#include "Application.h"
#include "CommandList.h"
#include "RootSignature.h"

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _heapType, uint32_t _numDescriptorsPerHeap)
    : descriptorHeapType(_heapType), numDescriptorsPerHeap(_numDescriptorsPerHeap), descriptorTableBitMask(0), staleDescriptorTableBitMask(0), currentCPUDescriptorHandle(D3D12_DEFAULT), currentGPUDescriptorHandle(D3D12_DEFAULT), numFreeHandles(0)
{
    auto device = Application::GetD3D12Device();
    // Since the increment size of a descriptor in a descriptor heap is vendor specific, it must be queried at runtime
    descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(_heapType);

    // Allocate space for staging CPU visible descriptors.
    descriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(numDescriptorsPerHeap);
}

void DynamicDescriptorHeap::ParseRootSignature(const RootSignature& rootSignature)
{
    // If the root signature changes, all descriptors must be (re)bound to the command list
    // This is reset to indicate that no descriptors should be copied to a GPU visible descriptor heap until new descriptors are staged to the DynamicDescriptorHeap
    staleDescriptorTableBitMask = 0;

    const auto& rootSignatureDesc = rootSignature.GetRootSignatureDesc();

    // Get a bit mask that represents the root parameter indices that match the descriptor heap type for this dynamic descriptor heap
    descriptorTableBitMask = rootSignature.GetDescriptorTableBitMask(descriptorHeapType);
    uint32_t DTBM = descriptorTableBitMask;

    uint32_t currentOffset = 0;
    DWORD rootIndex;
    while (_BitScanForward(&rootIndex, DTBM) && rootIndex < rootSignatureDesc.NumParameters)
    {
        uint32_t numDescriptors = rootSignature.GetNumDescriptors(rootIndex);

        DescriptorTableCache& DTC = descriptorTableCache[rootIndex];
        DTC.NumDescriptors = numDescriptors;
        DTC.BaseDescriptor = descriptorHandleCache.get() + currentOffset;

        currentOffset += numDescriptors;

        // Flip the descriptor table bit so it's not scanned again for the current index
        DTBM ^= (1 << rootIndex);
    }

    // Make sure the maximum number of descriptors per descriptor heap has not been exceeded
    assert(currentOffset <= numDescriptorsPerHeap && "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.");
}

void DynamicDescriptorHeap::StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
{
    // Cannot stage more than the maximum number of descriptors per heap
    // Cannot stage more than MaxDescriptorTables root parameters
    if (numDescriptors > numDescriptorsPerHeap || rootParameterIndex >= MaxDescriptorTables)
    {
        throw std::bad_alloc();
    }

    DescriptorTableCache& DTC = descriptorTableCache[rootParameterIndex];

    // Check that the number of descriptors to copy does not exceed the number of descriptors expected in the descriptor table
    if ((offset + numDescriptors) > DTC.NumDescriptors)
    {
        throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor table. Potential cause is using the wrong root parameter index when binding textures");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (DTC.BaseDescriptor + offset);
    for (uint32_t i = 0; i < numDescriptors; ++i)
    {
        dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptor, i, descriptorHandleIncrementSize);
    }

    // Set the root parameter index bit to make sure the descriptor table at that index is bound to the command list
    staleDescriptorTableBitMask |= (1 << rootParameterIndex);
}

uint32_t DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
    uint32_t numStaleDescriptors = 0;
    DWORD i;
    DWORD staleDescriptorsBitMask = staleDescriptorTableBitMask;

    while (_BitScanForward(&i, staleDescriptorsBitMask))
    {
        numStaleDescriptors += descriptorTableCache[i].NumDescriptors;
        staleDescriptorsBitMask ^= (1 << i);
    }

    return numStaleDescriptors;
}

ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::RequestDescriptorHeap()
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    // If there exists a descriptor heap, take it
    if (!availableDescriptorHeaps.empty())
    {
        descriptorHeap = availableDescriptorHeaps.front();
        availableDescriptorHeaps.pop();
    }
    else // else create a new one
    {
        descriptorHeap = CreateDescriptorHeap();
        descriptorHeapPool.push(descriptorHeap);
    }

    return descriptorHeap;
}

ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::CreateDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.Type = descriptorHeapType;
    descriptorHeapDesc.NumDescriptors = numDescriptorsPerHeap;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Enables descriptor to be mapped to the command list and used to access resources in a HLSL shader

    auto device = Application::GetD3D12Device();

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
{
    // Compute the number of descriptors that need to be copied 
    uint32_t numDescriptorsToCommit = ComputeStaleDescriptorCount();

    if (numDescriptorsToCommit > 0)
    {
        auto d3d12GraphicsCommandList = commandList.GetGraphicsCommandList().Get();
        assert(d3d12GraphicsCommandList != nullptr);

        if (!currentDescriptorHeap || numFreeHandles < numDescriptorsToCommit)
        {
            currentDescriptorHeap = RequestDescriptorHeap();
            currentCPUDescriptorHandle = currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            currentGPUDescriptorHandle = currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            numFreeHandles = numDescriptorsPerHeap;

            commandList.SetDescriptorHeap(descriptorHeapType, currentDescriptorHeap.Get());

            // When updating the descriptor heap on the command list, all descriptor tables must be (re)recopied to the new descriptor heap (not just the stale descriptor tables)
            staleDescriptorTableBitMask = descriptorTableBitMask;
        }

        auto device = Application::GetD3D12Device();

        DWORD rootIndex;
        // Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
        while (_BitScanForward(&rootIndex, staleDescriptorTableBitMask))
        {
            UINT numSrcDescriptors = descriptorTableCache[rootIndex].NumDescriptors;
            D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = descriptorTableCache[rootIndex].BaseDescriptor;

            D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] =
            {
                currentCPUDescriptorHandle
            };
            UINT pDestDescriptorRangeSizes[] =
            {
                numSrcDescriptors
            };

            // Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
            device->CopyDescriptors(1, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, numSrcDescriptors, pSrcDescriptorHandles, nullptr, descriptorHeapType);

            // Set the descriptors on the command list using the passed-in setter function.
            setFunc(d3d12GraphicsCommandList, rootIndex, currentGPUDescriptorHandle);

            // Offset current CPU and GPU descriptor handles.
            currentCPUDescriptorHandle.Offset(numSrcDescriptors, descriptorHandleIncrementSize);
            currentGPUDescriptorHandle.Offset(numSrcDescriptors, descriptorHandleIncrementSize);
            numFreeHandles -= numSrcDescriptors;

            // Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new descriptor.
            staleDescriptorTableBitMask ^= (1 << rootIndex);
        }
    }
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& commandList)
{
    CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& commandList)
{
    CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(CommandList& comandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
    if (!currentDescriptorHeap || numFreeHandles < 1)
    {
        currentDescriptorHeap = RequestDescriptorHeap();
        currentCPUDescriptorHandle = currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        currentGPUDescriptorHandle = currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        numFreeHandles = numDescriptorsPerHeap;

        comandList.SetDescriptorHeap(descriptorHeapType, currentDescriptorHeap.Get());

        // When updating the descriptor heap on the command list, all descriptor tables must be (re)recopied to the new descriptor heap (not just the stale descriptor tables)
        staleDescriptorTableBitMask = descriptorTableBitMask;
    }

    auto device = Application::GetD3D12Device();

    D3D12_GPU_DESCRIPTOR_HANDLE hGPU = currentGPUDescriptorHandle;
    device->CopyDescriptorsSimple(1, currentCPUDescriptorHandle, cpuDescriptor, descriptorHeapType);

    currentCPUDescriptorHandle.Offset(1, descriptorHandleIncrementSize);
    currentGPUDescriptorHandle.Offset(1, descriptorHandleIncrementSize);
    numFreeHandles -= 1;

    return hGPU;
}

void DynamicDescriptorHeap::Reset()
{
    availableDescriptorHeaps = descriptorHeapPool;
    currentDescriptorHeap.Reset();
    currentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    currentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    numFreeHandles = 0;
    descriptorTableBitMask = 0;
    staleDescriptorTableBitMask = 0;

    // Reset the table cache
    for (int i = 0; i < MaxDescriptorTables; ++i)
    {
        descriptorTableCache[i].Reset();
    }
}