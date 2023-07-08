#pragma once

#include "DescriptorAllocation.h"
#include "Resource.h"
#include <d3d12.h>
#include <memory>

class ShaderResourceView
{
public:
	std::shared_ptr<Resource> GetResource() const
	{
		return resource;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const
	{
		return descriptor.GetDescriptorHandle();
	}

	ShaderResourceView(const std::shared_ptr<Resource>& _resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);
	virtual ~ShaderResourceView() = default;

protected:
	std::shared_ptr<Resource> resource;
	DescriptorAllocation descriptor;
};