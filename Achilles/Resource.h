#pragma once

#include <string>
#include <memory>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

class Resource
{
public:
    Resource(const std::wstring& _name = L"");
    Resource(const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr, const std::wstring& _name = L"");
    Resource(ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr, const std::wstring& _name = L"");
    Resource(const Resource& copy);
    Resource(Resource&& copy);

    Resource& operator=(const Resource& other);
    Resource& operator=(Resource&& other);

    virtual ~Resource();

    // Check to see if the underlying resource is valid.
    bool IsValid() const
    {
        return (d3d12Resource != nullptr);
    }

    // Get access to the underlying D3D12 resource
    ComPtr<ID3D12Resource> GetD3D12Resource() const
    {
        return d3d12Resource;
    }

    D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
    {
        D3D12_RESOURCE_DESC resDesc = {};
        if (d3d12Resource)
        {
            resDesc = d3d12Resource->GetDesc();
        }

        return resDesc;
    }

    // Replace the D3D12 resource
    // Should only be called by the CommandList
    virtual void SetD3D12Resource(ComPtr<ID3D12Resource> d3d12Resource,
        const D3D12_CLEAR_VALUE* clearValue = nullptr);

    // Get the SRV for a resource.
    // @param srvDesc The description of the SRV to return. The default is nullptr which returns the default SRV for the resource (the SRV that is created when no description is provided.
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const = 0;

    // Get the UAV for a (sub)resource.
    // @param uavDesc The description of the UAV to return.
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const = 0;

    // Set the name of the resource. Useful for debugging purposes.
    // The name of the resource will persist if the underlying D3D12 resource is replaced with SetD3D12Resource.
    void SetName(const std::wstring& name);

    std::wstring GetName();

    // Release the underlying resource. This is useful for swap chain resizing.
    virtual void Reset();

protected:
    // The underlying D3D12 resource.
    ComPtr<ID3D12Resource> d3d12Resource;
    std::unique_ptr<D3D12_CLEAR_VALUE> d3d12ClearValue;
    std::wstring resourceName;
};