#include "Buffer.h"

Buffer::Buffer(const std::wstring& name) : Resource(name) {}

Buffer::Buffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSize, const std::wstring& _name) : Resource(_resDesc, nullptr, _name)
{
    CreateViews(_numElements, _elementSize);
}

Buffer::~Buffer()
{
    if (bufferData != nullptr)
        free(bufferData);
}

void Buffer::StoreBufferData(const void* _bufferData, size_t _bufferSize)
{
    bufferSize = _bufferSize;
    bufferData = malloc(bufferSize);
    memcpy_s(bufferData, bufferSize, _bufferData, bufferSize);
}

void* Buffer::GetBufferData()
{
    return bufferData;
}

size_t Buffer::GetBufferSize()
{
    return bufferSize;
}

void Buffer::CreateViews(size_t numElements, size_t elementSize)
{
    throw std::exception("Unimplemented function.");
}

bool Buffer::HasBeenCopied()
{
    return bufferInfo.hasBeenCopied;
}