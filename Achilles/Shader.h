#pragma once
#include "Common.h"
#include <directx-dxc/dxcapi.h>
#include "CommandList.h"

using Microsoft::WRL::ComPtr;

class Mesh;
class Camera;

typedef void (CALLBACK* ShaderRender)(std::shared_ptr<CommandList> commandList, std::shared_ptr<Mesh> mesh, std::shared_ptr<Camera> camera);

HRESULT CompileShader(std::wstring shaderPath, std::wstring entry, std::wstring profile, ComPtr<IDxcResult>& outShader);

struct Shader
{
	std::wstring name;
	D3D12_INPUT_ELEMENT_DESC* vertexLayout;
	size_t vertexSize;
	// Root signature
	std::shared_ptr<RootSignature> rootSignature;
	// Pipeline state object.
	ComPtr<ID3D12PipelineState> pipelineState;

	ShaderRender renderCallback;

	Shader(std::wstring _name, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, size_t _vertexSize, ShaderRender _renderCallback);
	// Presumes shader file is contains two entrypoints - VS + PS for vertex and pixel shaders respectively
	static std::shared_ptr<Shader> ShaderVSPS(ComPtr<ID3D12Device2> device, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, UINT vertexLayoutCount, size_t _vertexSize, std::shared_ptr<RootSignature> rootSignature, ShaderRender _renderCallback, std::wstring shaderName);
};

inline void ThrowBlobIfFailed(HRESULT hr, ComPtr<ID3DBlob> errorBlob)
{
	if (SUCCEEDED(hr))
		return;
	
	if (errorBlob == nullptr)
		throw std::exception(); // if you hit here then you might need to ThrowBlobIfFailed on a line after storing the HRESULT rather than one-lining it

	OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	throw std::exception();
}