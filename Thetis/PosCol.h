#pragma once
#include "Achilles/ShaderInclude.h"

using DirectX::XMMATRIX;
using DirectX::XMFLOAT3;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Color;

struct PosColVertex
{
	Vector3 Position;
	Color Color;
};

struct PosColCB0
{
	Matrix mvp;
};

static const PosColVertex posColQuadVertices[4] =
{
	{Vector3(-1.0f, -1.0f, +0.0f), Color(0.0f, 0.0f, 0.0f, 0.0f)},
	{Vector3(+1.0f, -1.0f, +0.0f), Color(1.0f, 0.0f, 0.0f, 0.0f)},
	{Vector3(-1.0f, +1.0f, +0.0f), Color(0.0f, 1.0f, 0.0f, 0.0f)},
	{Vector3(+1.0f, +1.0f, +0.0f), Color(1.0f, 1.0f, 0.0f, 0.0f)},
};

static const uint16_t posColQuadIndices[6] =
{
	0, 1, 2,
	2, 1, 3
};

static const PosColVertex posColCubeVertices[8] = {
	{ Vector3(-1.0f, -1.0f, -1.0f), Color(0.0f, 0.0f, 0.0f, 1.0f) },
	{ Vector3(-1.0f, +1.0f, -1.0f), Color(0.0f, 1.0f, 0.0f, 1.0f) },
	{ Vector3(+1.0f, +1.0f, -1.0f), Color(1.0f, 1.0f, 0.0f, 1.0f) },
	{ Vector3(+1.0f, -1.0f, -1.0f), Color(1.0f, 0.0f, 0.0f, 1.0f) },
	{ Vector3(-1.0f, -1.0f, +1.0f), Color(0.0f, 0.0f, 1.0f, 1.0f) },
	{ Vector3(-1.0f, +1.0f, +1.0f), Color(0.0f, 1.0f, 1.0f, 1.0f) },
	{ Vector3(+1.0f, +1.0f, +1.0f), Color(1.0f, 1.0f, 1.0f, 1.0f) },
	{ Vector3(+1.0f, -1.0f, +1.0f), Color(1.0f, 0.0f, 1.0f, 1.0f) }
};

static const uint16_t posColCubeIndices[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

static D3D12_INPUT_ELEMENT_DESC posColInputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

static void OutputDebugStringMatrix(Matrix mtx)
{
	OutputDebugStringWFormatted(L"% .02f % .02f % .02f % .02f\n% .02f % .02f % .02f % .02f\n% .02f % .02f % .02f % .02f\n% .02f % .02f % .02f % .02f\n\n", mtx._11, mtx._12, mtx._13, mtx._14, mtx._21, mtx._22, mtx._23, mtx._24, mtx._31, mtx._32, mtx._33, mtx._34, mtx._41, mtx._42, mtx._43, mtx._44);
}

void PosColShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera);
std::shared_ptr<Mesh> PosColMeshCreation(aiMesh* inMesh, std::shared_ptr<Shader> shader);
std::shared_ptr<Shader> GetPosColShader(ComPtr<ID3D12Device2> device = nullptr);