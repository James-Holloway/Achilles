#include "Buffer.h"

Buffer::Buffer(const std::wstring& name) : Resource(name) {}

Buffer::Buffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSize, const std::wstring& _name) : Resource(_resDesc, nullptr, _name)
{
    CreateViews(_numElements, _elementSize);
}

void Buffer::CreateViews(size_t numElements, size_t elementSize)
{
    throw std::exception("Unimplemented function.");
}