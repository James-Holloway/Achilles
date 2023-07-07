#include "ByteAddressBuffer.h"
#include "Application.h"


ByteAddressBuffer::ByteAddressBuffer(const std::wstring& name) : Buffer(name)
{
    SRV = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    UAV = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

ByteAddressBuffer::ByteAddressBuffer(const D3D12_RESOURCE_DESC& resDesc, size_t numElements, size_t elementSize, const std::wstring& name) : Buffer(resDesc, numElements, elementSize, name) {}

void ByteAddressBuffer::CreateViews(size_t numElements, size_t elementSize)
{
    auto device = Application::GetD3D12Device();

    // Make sure buffer size is aligned to 4 bytes.
    bufferSize = DirectX::AlignUp(numElements * elementSize, 4);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = (UINT)bufferSize / 4;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    device->CreateShaderResourceView(d3d12Resource.Get(), &srvDesc, SRV.GetDescriptorHandle());

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.Buffer.NumElements = (UINT)bufferSize / 4;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    device->CreateUnorderedAccessView(d3d12Resource.Get(), nullptr, &uavDesc, UAV.GetDescriptorHandle());
}