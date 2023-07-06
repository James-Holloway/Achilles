#pragma once
#include "Common.h"
using namespace Microsoft::WRL;

class Resources
{
public:
	static void UpdateBufferResource(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource> &destinationResource, ComPtr<ID3D12Resource>& intermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags);
	static void VertexUploadBufferCreateView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource>& destinationResource, ComPtr<ID3D12Resource>& intermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags, D3D12_VERTEX_BUFFER_VIEW& view);
	static void IndexUploadBufferCreateView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource>& destinationResource, ComPtr<ID3D12Resource>& intermediateResource, size_t numElements, const void* bufferData, D3D12_RESOURCE_FLAGS flags, D3D12_INDEX_BUFFER_VIEW& view);
};