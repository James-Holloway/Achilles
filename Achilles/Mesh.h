#pragma once
#include "CommandQueue.h"
#include "CommandList.h"
#include "Material.h"

using Microsoft::WRL::ComPtr;

class Mesh
{
protected:
	std::wstring name = L"Unnamed Mesh";
	bool isCreated = false;
	std::shared_ptr<VertexBuffer> vertexBuffer;
	std::shared_ptr<IndexBuffer> indexBuffer;
	DirectX::BoundingBox boundingBox;
	std::shared_ptr<Shader> shader;
	D3D_PRIMITIVE_TOPOLOGY topology;

public:
	// Only allows D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST at the moment
	Mesh(std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader);
	Mesh(std::wstring _name, std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader);
	std::wstring GetName();
	void SetName(std::wstring newName);

	DirectX::BoundingBox GetBoundingBox();
	void SetBoundingBox(DirectX::BoundingBox box);

	std::shared_ptr<VertexBuffer> GetVertexBuffer();
	std::shared_ptr<IndexBuffer> GetIndexBuffer();
	D3D_PRIMITIVE_TOPOLOGY GetTopology();
	std::shared_ptr<Shader> GetShader();
	uint32_t GetNumIndices();

	// Gets the position of every vertex. Duplicated points
	std::vector<DirectX::SimpleMath::Vector3> GetTrianglePoints(DirectX::SimpleMath::Matrix transformMatrix = DirectX::SimpleMath::Matrix::Identity);

	bool HasBeenCopied();
};
