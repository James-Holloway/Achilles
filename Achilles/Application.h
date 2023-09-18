#pragma once

#include <cstdint>
#include <memory>
#include "d3d12.h"
#include "DescriptorAllocator.h"

using Microsoft::WRL::ComPtr;

class CommandQueue;

enum class MSAA
{
    Off,
    x2,
    x4,
    x8,
};

class Application
{
private:
    inline static uint64_t globalFrameCounter = 0;
    inline static float timeElapsed = 0;

    inline static ComPtr<ID3D12Device2> d3d12Device = nullptr;

    inline static std::shared_ptr<CommandQueue> directCommandQueue = nullptr;
    inline static std::shared_ptr<CommandQueue> computeCommandQueue = nullptr;
    inline static std::shared_ptr<CommandQueue> copyCommandQueue = nullptr;

    inline static std::unique_ptr<DescriptorAllocator> descriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { nullptr };

    inline static bool isEditor = false;

    inline static MSAA msaa = MSAA::Off;
    inline static DXGI_SAMPLE_DESC msaaFormat = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 };

public:
    // Global Frame Counter
    inline static uint64_t GetGlobalFrameCounter()
    {
        return globalFrameCounter;
    }

    inline static void IncrementGlobalFrameCounter()
    {
        globalFrameCounter++;
    }

    inline static float GetTimeElapsed()
    {
        return timeElapsed;
    }

    inline static void UpdateTimeElapsed(float newTimeElapsed)
    {
        timeElapsed = newTimeElapsed;
    }

    // D3D12Device
    inline static void SetD3D12Device(ComPtr<ID3D12Device2> device)
    {
        d3d12Device = device;
    }
    inline static ComPtr<ID3D12Device2> GetD3D12Device()
    {
        return d3d12Device;
    }
    inline static void ResetD3D12Device()
    {
        if (d3d12Device)
        {
            d3d12Device.Reset();
            d3d12Device = nullptr;
        }
    }

    // Returns the MSAA sampling required for DXGI_SAMPLE_DESC
    static DXGI_SAMPLE_DESC GetSampleDescription();

    static bool SetMSAASample(MSAA _msaa);
    static MSAA GetMSAA();

    // Descriptor Allocators
    // Allocate a number of CPU visible descriptors.
    static DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);

    // Release stale descriptors. This should only be called with a completed frame counter.
    static void ReleaseStaleDescriptors(uint64_t finishedFrame);

    static ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);
    static UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type);

    // Create the descriptor allocators
    static void CreateDescriptorAllocators();

    // Command Queues
    static std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    static void SetCommandQueue(D3D12_COMMAND_LIST_TYPE type, std::shared_ptr<CommandQueue> commandQueue);
    static std::shared_ptr<CommandQueue> GetNewCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);


    // Multisampling
    static DXGI_SAMPLE_DESC GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT, D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE);

    static bool IsEditor();
    static void SetIsEditor(bool _isEditor);
};
