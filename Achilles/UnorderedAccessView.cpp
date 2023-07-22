#include "UnorderedAccessView.h"
#include "Application.h"

UnorderedAccessView::UnorderedAccessView(const std::shared_ptr<Resource>& _resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav) : resource(_resource)
{
    assert(resource || uav);

    auto d3d12Resource = resource ? resource->GetD3D12Resource() : nullptr;
    auto d3d12Device = Application::GetD3D12Device();

    descriptor = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    d3d12Device->CreateUnorderedAccessView(d3d12Resource.Get(), nullptr, uav, descriptor.GetDescriptorHandle());
}