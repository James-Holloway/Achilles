#include "Resource.h"
#include "Application.h"
#include "ResourceStateTracker.h"

Resource::Resource(const std::wstring& name) : resourceName(name) {}

Resource::Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue, const std::wstring& name)
{
    if (clearValue)
    {
        d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }

    auto device = Application::GetD3D12Device();

    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, d3d12ClearValue.get(), IID_PPV_ARGS(&d3d12Resource)));

    ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

    SetName(name);
}

Resource::Resource(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* _clearValue, const std::wstring& name) : d3d12Resource(resource)
{
    if (_clearValue)
    {
        d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*_clearValue);
    }

    SetName(name);
}

Resource::Resource(const Resource& copy) : d3d12Resource(copy.d3d12Resource), resourceName(copy.resourceName)
{
    if (copy.d3d12ClearValue)
    {
        d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*copy.d3d12ClearValue);
    }
}

Resource::Resource(Resource&& copy) : d3d12Resource(std::move(copy.d3d12Resource)), resourceName(std::move(copy.resourceName)), d3d12ClearValue(std::move(copy.d3d12ClearValue)) { }

Resource& Resource::operator=(const Resource& other)
{
    if (this != &other)
    {
        d3d12Resource = other.d3d12Resource;
        resourceName = other.resourceName;
        if (other.d3d12ClearValue)
        {
            d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*other.d3d12ClearValue);
        }
    }

    return *this;
}

Resource& Resource::operator=(Resource&& other)
{
    if (this != &other)
    {
        d3d12Resource = other.d3d12Resource;
        resourceName = other.resourceName;
        d3d12ClearValue = std::move(other.d3d12ClearValue);

        other.d3d12Resource.Reset();
        other.resourceName.clear();
    }

    return *this;
}


Resource::~Resource()
{
}

void Resource::SetD3D12Resource(ComPtr<ID3D12Resource> _d3d12Resource, const D3D12_CLEAR_VALUE* clearValue)
{
    d3d12Resource = _d3d12Resource;
    if (d3d12ClearValue)
    {
        d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }
    else
    {
        d3d12ClearValue.reset();
    }
    SetName(resourceName);
}

void Resource::SetName(const std::wstring& name)
{
    resourceName = name;
    if (d3d12Resource && !resourceName.empty())
    {
        d3d12Resource->SetName(resourceName.c_str());
    }
}

void Resource::Reset()
{
    d3d12Resource.Reset();
    d3d12ClearValue.reset();
}