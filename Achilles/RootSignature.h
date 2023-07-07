#pragma once

#include "Common.h"

/*
 *  Copyright(c) 2018 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

using Microsoft::WRL::ComPtr;

class RootSignature
{
public:
    RootSignature();
    RootSignature(ComPtr<ID3D12Device2> _device, const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION _rootSignatureVersion);

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

    ComPtr<ID3D12Device2> device;
};