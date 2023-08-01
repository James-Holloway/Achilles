#pragma once

#include <cstdint>
#include <d3d12.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class RootSignature
{
public:
    RootSignature();
    RootSignature(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION _rootSignatureVersion);

    virtual ~RootSignature();

    void Destroy();

    ComPtr<ID3D12RootSignature> GetRootSignature() const
    {
        return rootSignature;
    }

    void SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION _rootSignatureVersion);

    const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const
    {
        return rootSignatureDesc;
    }

    uint32_t GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;
    uint32_t GetNumDescriptors(uint32_t rootIndex) const;

protected:

private:
    D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc;
    ComPtr<ID3D12RootSignature> rootSignature;

    // Need to know the number of descriptors per descriptor table
    // A maximum of 32 descriptor tables are supported (since a 32-bit mask is used to represent the descriptor tables in the root signature)
    uint32_t numDescriptorsPerTable[32];

    // A bit mask that represents the root parameter indices that are descriptor tables for Samplers
    uint32_t samplerTableBitMask;
    // A bit mask that represents the root parameter indices that are CBV, UAV, and SRV descriptor tables
    uint32_t descriptorTableBitMask;
};