#pragma once

#include "Common.h"
#include "Resource.h"
#include "BufferInfo.h"

class Buffer : public Resource
{
public:
    Buffer(const std::wstring& _name = L"");
    Buffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSize, const std::wstring& _name = L"");

    BufferInfo bufferInfo{};

    virtual void CreateViews(size_t numElements, size_t elementSize) = 0;
    bool HasBeenCopied();
};
