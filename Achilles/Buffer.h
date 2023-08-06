#pragma once

#include "Resource.h"
#include "BufferInfo.h"

class Buffer : public Resource
{
public:
    Buffer(const std::wstring& _name = L"");
    Buffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSize, const std::wstring& _name = L"");
    virtual ~Buffer();

    BufferInfo bufferInfo{};

    void* bufferData = nullptr;
    size_t bufferSize = 0;

    virtual void StoreBufferData(const void* _bufferData, size_t _bufferSize);
    virtual void* GetBufferData();
    virtual size_t GetBufferSize();

    virtual void CreateViews(size_t numElements, size_t elementSize) = 0;
    bool HasBeenCopied();
};
