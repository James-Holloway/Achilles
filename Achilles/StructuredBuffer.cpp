#include "StructuredBuffer.h"

#include "Application.h"
#include "ResourceStateTracker.h"

#include <d3dx12.h>

StructuredBuffer::StructuredBuffer(const std::wstring& name) : Buffer(name), counterBuffer(CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), 1, 4, name + L" Counter"), numElements(0), elementSize(0)
{
    SRV = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    UAV = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

StructuredBuffer::StructuredBuffer(const D3D12_RESOURCE_DESC& resDesc, size_t _numElements, size_t _elementSize, const std::wstring& name)
    : Buffer(resDesc, _numElements, _elementSize, name), counterBuffer(CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),1, 4, name + L" Counter"), numElements(_numElements), elementSize(_elementSize)
{
    SRV = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    UAV = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void StructuredBuffer::CreateViews(size_t _numElements, size_t _elementSize)
{
    auto device = Application::GetD3D12Device();

    numElements = _numElements;
    elementSize = _elementSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = static_cast<UINT>(numElements);
    srvDesc.Buffer.StructureByteStride = static_cast<UINT>(elementSize);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(d3d12Resource.Get(), &srvDesc, SRV.GetDescriptorHandle());

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.NumElements = static_cast<UINT>(numElements);
    uavDesc.Buffer.StructureByteStride = static_cast<UINT>(elementSize);
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    device->CreateUnorderedAccessView(d3d12Resource.Get(), counterBuffer.GetD3D12Resource().Get(), &uavDesc, UAV.GetDescriptorHandle());
}