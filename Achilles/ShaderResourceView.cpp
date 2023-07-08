#include "ShaderResourceView.h"

#include "Application.h"

ShaderResourceView::ShaderResourceView(const std::shared_ptr<Resource>& _resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv) : resource(_resource)
{
    assert(resource || srv);

    auto d3d12Resource = resource ? resource->GetD3D12Resource() : nullptr;
    auto d3d12Device = Application::GetD3D12Device();

    descriptor = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    d3d12Device->CreateShaderResourceView(d3d12Resource.Get(), srv, descriptor.GetDescriptorHandle());
}