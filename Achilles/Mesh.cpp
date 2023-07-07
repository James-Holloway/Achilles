#include "Mesh.h"
#include "ShaderInclude.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;


Mesh::Mesh(std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader)
{
	topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	vertexBuffer = std::make_shared<VertexBuffer>(L"Mesh Vertex Buffer");
	indexBuffer = std::make_shared<IndexBuffer>(L"Mesh Index Buffer");

	commandList->CopyVertexBuffer(*vertexBuffer, vertexCount, vertexStride, vertices);
	commandList->CopyIndexBuffer(*indexBuffer, indexCount, DXGI_FORMAT_R16_UINT, indices);

	shader = _shader;

	isCreated = true;
}

Mesh::Mesh(std::wstring _name, std::shared_ptr<CommandList> commandList, void* vertices, UINT vertexCount, size_t vertexStride, const uint16_t* indices, UINT indexCount, std::shared_ptr<Shader> _shader)
	: Mesh(commandList, vertices, vertexCount, vertexStride, indices, indexCount, _shader)
{
	name = _name;
}

void Mesh::ConstructMatrix()
{
	// TRS
	matrix = Matrix::CreateScale(scale) * (Matrix::CreateFromYawPitchRoll(rotation) * Matrix::CreateTranslation(position));
}

DirectX::SimpleMath::Matrix Mesh::GetMatrix()
{
	ConstructMatrix();
	return matrix;
}