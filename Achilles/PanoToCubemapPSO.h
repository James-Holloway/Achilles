#pragma once

#include "RootSignature.h"
#include "DescriptorAllocation.h"

#include <cstdint>

using Microsoft::WRL::ComPtr;

// Struct used in the PanoToCubemap_CS compute shader.
struct PanoToCubemapCB
{
    // Size of the cubemap face in pixels at the current mipmap level.
    uint32_t CubemapSize;
    // The first mip level to generate.
    uint32_t FirstMip;
    // The number of mips to generate.
    uint32_t NumMips;
};

// I don't use scoped enums to avoid the explicit cast that is required to 
// treat these as root indices into the root signature.
namespace PanoToCubemapRS
{
    enum
    {
        PanoToCubemapCB,
        SrcTexture,
        DstMips,
        NumRootParameters
    };
}

class PanoToCubemapPSO
{
public:
    PanoToCubemapPSO();

    const RootSignature& GetRootSignature() const
    {
        return rootSignature;
    }

    ComPtr<ID3D12PipelineState> GetPipelineState() const
    {
        return pipelineState;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const
    {
        return defaultUAV.GetDescriptorHandle();
    }

private:
    RootSignature rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    // Default (no resource) UAV's to pad the unused UAV descriptors.
    // If generating less than 5 mip map levels, the unused mip maps need to be padded with default UAVs (to keep the DX12 runtime happy).
    DescriptorAllocation defaultUAV;
};