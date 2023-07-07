#include "ConstantBuffer.h"
#include "Application.h"

ConstantBuffer::ConstantBuffer(const std::wstring& _name) : Buffer(_name), sizeInBytes(0)
{
    constantBufferView = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

ConstantBuffer::~ConstantBuffer()
{}

void ConstantBuffer::CreateViews(size_t numElements, size_t elementSize)
{
    sizeInBytes = numElements * elementSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc;
    d3d12ConstantBufferViewDesc.BufferLocation = d3d12Resource->GetGPUVirtualAddress();
    d3d12ConstantBufferViewDesc.SizeInBytes = static_cast<UINT>(DirectX::AlignUp(sizeInBytes, 16));

    auto device = Application::GetD3D12Device();

    device->CreateConstantBufferView(&d3d12ConstantBufferViewDesc, constantBufferView.GetDescriptorHandle());
}

D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    throw std::exception("ConstantBuffer::GetShaderResourceView should not be called.");
}

D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    throw std::exception("ConstantBuffer::GetUnorderedAccessView should not be called.");
}