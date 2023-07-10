#pragma once
#pragma once
#include "Achilles/ShaderInclude.h"

using DirectX::XMMATRIX;
using DirectX::XMFLOAT3;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Color;

namespace PosTextured
{
	struct PosTexturedVertex
	{
		Vector3 Position;
		Vector2 UV;
	};


	struct alignas(16) PosTexturedCB0
	{
		Matrix mvp;
	};

	static const PosTexturedVertex posTexturedQuadVertices[4] =
	{
		{Vector3(-1.0f, -1.0f, +0.0f), Vector2(0.0f, 0.0f)},
		{Vector3(+1.0f, -1.0f, +0.0f), Vector2(1.0f, 0.0f)},
		{Vector3(-1.0f, +1.0f, +0.0f), Vector2(0.0f, 1.0f)},
		{Vector3(+1.0f, +1.0f, +0.0f), Vector2(1.0f, 1.0f)},
	};

	static const uint16_t posTexturedQuadIndices[6] =
	{
		0, 1, 2,
		2, 1, 3
	};

	static const PosTexturedVertex posTexturedCubeVertices[8] = {
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(-1.0f, +1.0f, -1.0f), Vector2(0.0f, 1.0f) },
		{ Vector3(+1.0f, +1.0f, -1.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(+1.0f, -1.0f, -1.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, +1.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(-1.0f, +1.0f, +1.0f), Vector2(0.0f, 1.0f) },
		{ Vector3(+1.0f, +1.0f, +1.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(+1.0f, -1.0f, +1.0f), Vector2(1.0f, 0.0f) }
	};

	static const uint16_t posTexturedCubeIndices[36] =
	{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	enum RootParameters
	{
		//// Vertex shader parameter ////
		RootParameterPosTexturedCB0,  // ConstantBuffer<PosTexturedCB0> MatCB : register(b0);

		//// Pixel shader parameters ////
		RootParameterTextures,
		// Texture2D MainTexture : register( t0 );

		RootParameterCount
	};

	static D3D12_INPUT_ELEMENT_DESC posTexturedInputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	inline void PosTexturedShaderRender(std::shared_ptr<CommandList> commandList, std::shared_ptr<Object> object, std::shared_ptr<Mesh> mesh, Material material, std::shared_ptr<Camera> camera)
	{
		// Update the MVP matrix
		Matrix mvp = object->GetWorldMatrix() * (camera->GetView() * camera->GetProj());

		PosTexturedCB0 cb0{ mvp };

		std::shared_ptr<Texture> mainTexture = nullptr;

		auto mainTextureIter = material.textures.find(L"MainTexture");
		if (mainTextureIter != material.textures.end())
			mainTexture = mainTextureIter->second;

		material.shader->BindTexture(*commandList, RootParameters::RootParameterTextures, 0, mainTexture);

		commandList->SetGraphics32BitConstants<PosTexturedCB0>(0, cb0);
	}

	static std::shared_ptr<Shader> posTexturedShader{};
	static CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC polTexturedRootSignature{};
	inline std::shared_ptr<Shader> GetPosTexturedShader(ComPtr<ID3D12Device2> device)
	{
		if (posTexturedShader.use_count() >= 1)
		{
			return posTexturedShader;
		}

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// Textures descriptor range
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 texture, offset at 0, in space 0

		// Root parameters
		CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::RootParameterCount]{};
		rootParameters[RootParameters::RootParameterPosTexturedCB0].InitAsConstants(sizeof(PosTexturedCB0) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[RootParameters::RootParameterTextures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		// Sampler(s)
		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 8U); // anisotropic sampler set to 8

		polTexturedRootSignature.Init_1_1(_countof(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags); // 1 is number of static samplers

		std::shared_ptr rootSignature = std::make_shared<RootSignature>(polTexturedRootSignature.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

		posTexturedShader = Shader::ShaderVSPS(device, posTexturedInputLayout, _countof(posTexturedInputLayout), sizeof(PosTexturedVertex), rootSignature, PosTexturedShaderRender, L"PosTextured");

		return posTexturedShader;
	}
}