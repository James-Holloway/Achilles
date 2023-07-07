#include "Application.h"
#include "CommandQueue.h"

DescriptorAllocation Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    return descriptorAllocators[type]->Allocate(numDescriptors);
}

void Application::ReleaseStaleDescriptors(uint64_t finishedFrame)
{
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        descriptorAllocators[i]->ReleaseStaleDescriptors(finishedFrame);
    }
}

ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    ThrowIfFailed(d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return d3d12Device->GetDescriptorHandleIncrementSize(type);
}

void Application::CreateDescriptorAllocators()
{
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        descriptorAllocators[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
    }
}

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        return directCommandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        return computeCommandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        return copyCommandQueue;
        break;
    }
    throw std::exception("Command queue currently not supported");
}

void Application::SetCommandQueue(D3D12_COMMAND_LIST_TYPE type, std::shared_ptr<CommandQueue> commandQueue)
{
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        directCommandQueue = commandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        computeCommandQueue = commandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        copyCommandQueue = commandQueue;
        break;
    }
}

DXGI_SAMPLE_DESC Application::GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples, D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags)
{
    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
    qualityLevels.Format = format;
    qualityLevels.SampleCount = 1;
    qualityLevels.Flags = flags;
    qualityLevels.NumQualityLevels = 0;

    while (qualityLevels.SampleCount <= numSamples && SUCCEEDED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS))) && qualityLevels.NumQualityLevels > 0)
    {
        // That works...
        sampleDesc.Count = qualityLevels.SampleCount;
        sampleDesc.Quality = qualityLevels.NumQualityLevels - 1;

        // But can we do better?
        qualityLevels.SampleCount *= 2;
    }

    return sampleDesc;
}