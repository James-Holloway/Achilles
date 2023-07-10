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


inline void PosColShaderRender(std::shared_ptr<CommandList> commandList, Mesh* mesh, Material material, std::shared_ptr<Camera> camera)
{
	// Update the MVP matrix
	Matrix mvp = mesh->GetMatrix() * (camera->GetView() * camera->GetProj());

	PosColCB0 cb0{ mvp };

	commandList->SetGraphics32BitConstants<PosColCB0>(0, cb0);
}

static std::shared_ptr<Shader> posColShader{};
static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC polColRootSignature{};
inline std::shared_ptr<Shader> GetPosColShader(ComPtr<ID3D12Device2> device)
{
	if (posColShader.use_count() >= 1)
	{
		return posColShader;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameters[1]{};
	rootParameters[0].InitAsConstants(sizeof(PosColCB0) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	polColRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	std::shared_ptr rootSignature = std::make_shared<RootSignature>(polColRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

	posColShader = Shader::ShaderVSPS(device, posColInputLayout, _countof(posColInputLayout), sizeof(PosColVertex), rootSignature, PosColShaderRender, L"PosCol");

	return posColShader;
}