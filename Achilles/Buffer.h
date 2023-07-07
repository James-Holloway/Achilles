#pragma once

#include "Common.h"
#include "Resource.h"

class Buffer : public Resource
{
public:
	Buffer(const std::wstring& _name = L"");
	Buffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSize, const std::wstring& _name = L"");

	virtual void CreateViews(size_t numElements, size_t elementSize) = 0;
};
