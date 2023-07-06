#include "Resources.h"

void Resources::UpdateBufferResource(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource>& destinationResource, ComPtr<ID3D12Resource>& intermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	size_t bufferSize = numElements * elementSize;

	// Create a committed resource for the GPU resource in a default heap.
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&destinationResource)));

	// Create an committed resource for the upload to GPU
	if (bufferData)
	{
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		intermediateResource->SetName(L"Intermediate Copy Buffer");

		::UpdateSubresources(commandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, 0, 1, &subresourceData);
	}
}

void Resources::VertexUploadBufferCreateView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource>& destinationResource, ComPtr<ID3D12Resource>& intermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags, D3D12_VERTEX_BUFFER_VIEW& view)
{
	UpdateBufferResource(device, commandList, destinationResource, intermediateResource, numElements, elementSize, bufferData, flags);
	view.BufferLocation = destinationResource->GetGPUVirtualAddress();
	view.SizeInBytes = (UINT)(elementSize * numElements);
	view.StrideInBytes = (UINT)elementSize;
	destinationResource->SetName(L"VertexBuffer");
}

void Resources::IndexUploadBufferCreateView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource>& destinationResource, ComPtr<ID3D12Resource>& intermediateResource, size_t numElements, const void* bufferData, D3D12_RESOURCE_FLAGS flags, D3D12_INDEX_BUFFER_VIEW& view)
{
	size_t elementSize = sizeof(uint16_t);
	UpdateBufferResource(device, commandList, destinationResource, intermediateResource, numElements, elementSize, bufferData, flags);
	view.BufferLocation = destinationResource->GetGPUVirtualAddress();
	view.Format = DXGI_FORMAT_R16_UINT; // uint16_t
	view.SizeInBytes = (UINT)(elementSize * numElements);
	destinationResource->SetName(L"IndexBuffer");
}