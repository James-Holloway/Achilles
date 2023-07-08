#include "ConstantBufferView.h"

#include "MathHelpers.h"
#include "ConstantBuffer.h"
#include "Application.h"

ConstantBufferView::ConstantBufferView(const std::shared_ptr<ConstantBuffer>& _constantBuffer, size_t offset) : constantBuffer(_constantBuffer)
{
    assert(constantBuffer);

    auto d3d12Device = Application::GetD3D12Device();
    auto d3d12Resource = constantBuffer->GetD3D12Resource();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
    cbv.BufferLocation = d3d12Resource->GetGPUVirtualAddress() + offset;
    cbv.SizeInBytes = DirectX::AlignUp((UINT)constantBuffer->GetSizeInBytes(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);  // Constant buffers must be aligned for hardware requirements.

    descriptor = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    d3d12Device->CreateConstantBufferView(&cbv, descriptor.GetDescriptorHandle());
}