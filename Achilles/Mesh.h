#pragma once
#include "Common.h"
#include "Resources.h"
#include "Shader.h"

using Microsoft::WRL::ComPtr;

class Mesh
{
	friend class Achilles;
public:
	std::wstring name = L"Unnamed Mesh";
	DirectX::SimpleMath::Vector3 position {0, 0, 0};
	DirectX::SimpleMath::Vector3 rotation {0, 0, 0};
	DirectX::SimpleMath::Vector3 scale {1, 1, 1};

protected:
	DirectX::SimpleMath::Matrix matrix;

	bool isCreated = false;
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	UINT vertexCount;
	ComPtr<ID3D12Resource>* vertexIntermediateBuffer = new ComPtr<ID3D12Resource>();

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	UINT indexCount;
	ComPtr<ID3D12Resource>* indexIntermediateBuffer;

	std::shared_ptr<Shader> shader;

	D3D_PRIMITIVE_TOPOLOGY topology;

public:
	// Only allows D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST at the moment
	Mesh(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, void* vertices, UINT _vertexCount, const uint16_t* indices, UINT _indexCount, std::shared_ptr<Shader> _shader);
	Mesh(std::wstring _name, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, void* vertices, UINT _vertexCount, const uint16_t* indices, UINT _indexCount, std::shared_ptr<Shader> _shader);
	void FreeCreationResources(); // Needs to be called once the command list has been completed

protected:
	void ConstructMatrix();

public:
	DirectX::SimpleMath::Matrix GetMatrix();
};
