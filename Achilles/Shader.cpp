#include "Shader.h"

HRESULT CompileShader(std::wstring shaderPath, std::wstring entry, std::wstring profile, ComPtr<IDxcBlob>& outShader)
{
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;
	ThrowIfFailed(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
	ThrowIfFailed(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

	ComPtr<IDxcIncludeHandler> includeHandler;
	ThrowIfFailed(utils->CreateDefaultIncludeHandler(&includeHandler));

	std::vector<LPCWSTR> compilationArguments
	{
		shaderPath.c_str(), L"-E", entry.c_str(), L"-T", profile.c_str(), /*DXC_ARG_PACK_MATRIX_ROW_MAJOR,*/ DXC_ARG_WARNINGS_ARE_ERRORS, DXC_ARG_ALL_RESOURCES_BOUND
	};

	// Indicate that the shader should be in a debuggable state if in debug mode
#if _DEBUG
	compilationArguments.push_back(L"-Zs"); // Enable debug information (slim format)
#else
	compilationArguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

	ComPtr<IDxcBlobEncoding> source = nullptr;
	utils->LoadFile(shaderPath.c_str(), nullptr, &source);

	DxcBuffer sourceBuffer
	{
		.Ptr = source->GetBufferPointer(),
		.Size = source->GetBufferSize(),
		.Encoding = DXC_CP_ACP,
	};

	ComPtr<IDxcResult> compiledShader{};
	HRESULT hr = compiler->Compile(&sourceBuffer, compilationArguments.data(), static_cast<uint32_t>(compilationArguments.size()), includeHandler.Get(), IID_PPV_ARGS(&compiledShader));

	if (FAILED(hr))
	{
		const std::wstring errorText = L"Failed to compile shader with path : " + shaderPath + L"\n";
		OutputDebugStringW(errorText.c_str());
		return E_FAIL;
	}

	// Get compilation errors (if any).
	ComPtr<IDxcBlobUtf8> errors{};
	ThrowIfFailed(compiledShader->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
	if (errors && errors->GetStringLength() > 0)
	{
		const LPCSTR errorMessage = errors->GetStringPointer();
		OutputDebugStringA(errorMessage);
	}

	HRESULT hrStatus;
	if (FAILED(compiledShader->GetStatus(&hrStatus)) || FAILED(hrStatus))
	{
		wchar_t outBuffer[MAX_PATH + 96];
		swprintf_s(outBuffer, L"Failed to get compile shader (0x%X) : %s\n", hrStatus, shaderPath.c_str());
		OutputDebugStringW(outBuffer);
		return hrStatus;
	}

	ComPtr<IDxcBlob> shader;
	ComPtr<IDxcBlobWide> shaderName;
	hrStatus = compiledShader->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), &shaderName);
	if (FAILED(hrStatus))
	{
		wchar_t outBuffer[MAX_PATH + 96];
		swprintf_s(outBuffer, L"Failed to get shader output (0x%X) : %s\n", hrStatus, shaderPath.c_str());
		OutputDebugStringW(outBuffer);
		return hrStatus;
	}

	outShader = shader;

	return S_OK;
}

Shader::Shader(std::wstring _name, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, size_t _vertexSize, ShaderRender _renderCallback)
{
	name = _name;
	vertexLayout = _vertexLayout;
	vertexSize = _vertexSize;
	renderCallback = _renderCallback;
}

std::shared_ptr<Shader> Shader::ShaderVSPS(ComPtr<ID3D12Device2> device, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, UINT vertexLayoutCount, size_t _vertexSize, CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription, ShaderRender _renderCallback, std::wstring shaderName)
{
	std::shared_ptr<Shader> shader = std::make_shared<Shader>(shaderName, _vertexLayout, _vertexSize, _renderCallback);

	std::wstring shaderPath = GetContentDirectoryW() + L"shaders/" + shaderName + L".hlsl";

	// Compile shaders at runtime

	ComPtr<IDxcBlob> vertexShader;
	ComPtr<IDxcBlob> pixelShader;
	ThrowIfFailed(CompileShader(shaderPath, L"VS", L"vs_6_4", vertexShader));
	ThrowIfFailed(CompileShader(shaderPath, L"PS", L"ps_6_4", pixelShader));

	if (vertexShader == nullptr || pixelShader == nullptr)
		throw std::exception("Shader(s) did not compile");

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob);
	ThrowBlobIfFailed(hr, errorBlob);

	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&shader->rootSignature)));

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = shader->rootSignature.Get();
	pipelineStateStream.InputLayout = { _vertexLayout, vertexLayoutCount };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};

	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&shader->pipelineState)));

	shader->pipelineState->SetName(shaderName.c_str());

	return shader;
}