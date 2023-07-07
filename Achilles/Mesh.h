#pragma once
#include "Common.h"
#include "CommandList.h"
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
	std::shared_ptr<VertexBuffer> vertexBuffer;
	std::shared_ptr<IndexBuffer> indexBuffer;

	std::shared_ptr<Shader> shader;

	D3D_PRIMITIVE_TOPOLOGY topology;

public:
	// Only allows D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST at the moment
	Mesh(std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader);
	Mesh(std::wstring _name, std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader);

protected:
	void ConstructMatrix();

public:
	DirectX::SimpleMath::Matrix GetMatrix();
};
