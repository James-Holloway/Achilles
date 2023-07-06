#include "Mesh.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Mesh::Mesh(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, void* vertices, UINT _vertexCount, const uint16_t* indices, UINT _indexCount, std::shared_ptr<Shader> _shader)
{
	topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	vertexCount = _vertexCount;
	indexCount = _indexCount;
	shader = _shader;

	vertexIntermediateBuffer = new ComPtr<ID3D12Resource>();
	indexIntermediateBuffer = new ComPtr<ID3D12Resource>();

	Resources::VertexUploadBufferCreateView(device, commandList, vertexBuffer, *vertexIntermediateBuffer, vertexCount, shader->vertexSize, vertices, D3D12_RESOURCE_FLAG_NONE, vertexBufferView);
	Resources::IndexUploadBufferCreateView(device, commandList, indexBuffer, *indexIntermediateBuffer, indexCount, indices, D3D12_RESOURCE_FLAG_NONE, indexBufferView);
	isCreated = true;
}

Mesh::Mesh(std::wstring _name, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, void* vertices, UINT vertexCount, const uint16_t* indices, UINT _indexCount, std::shared_ptr<Shader> _shader) :
	Mesh(device, commandList, vertices, vertexCount, indices, _indexCount, _shader)
{
	name = _name;
}

void Mesh::FreeCreationResources()
{
	delete vertexIntermediateBuffer;
	delete indexIntermediateBuffer;
}

void Mesh::ConstructMatrix()
{
	// TRS
	matrix = (Matrix::CreateTranslation(position) * Matrix::CreateFromQuaternion(rotation)) * Matrix::CreateScale(scale);
}

DirectX::SimpleMath::Matrix Mesh::GetMatrix()
{
	ConstructMatrix();
	return matrix;
}