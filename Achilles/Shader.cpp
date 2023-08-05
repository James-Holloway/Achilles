#include "Shader.h"
#include "Application.h"
#include "RootSignature.h"
#include "Texture.h"

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
#if defined(_DEBUG) || defined(_UNOPTIMIZED)
    compilationArguments.push_back(L"-Zs"); // Enable debug information (slim format)
#else
    compilationArguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

    ComPtr<IDxcBlobEncoding> source = nullptr;

    std::error_code ec;
    if (!std::filesystem::exists(shaderPath, ec))
    {
        throw std::exception("File does not exist");
    }

    ThrowIfFailed(utils->LoadFile(shaderPath.c_str(), nullptr, &source));

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
        swprintf_s(outBuffer, L"Failed to compile shader (0x%X) : %s\n", hrStatus, shaderPath.c_str());
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

Shader::Shader(std::wstring _name) : name(_name), vertexLayout(nullptr), vertexSize(0)
{

}

Shader::Shader(std::wstring _name, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, size_t _vertexSize) : name(_name), vertexLayout(_vertexLayout), vertexSize(_vertexSize)
{

}

Shader::Shader(std::wstring _name, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, size_t _vertexSize, ShaderRender _renderCallback) : name(_name), vertexLayout(_vertexLayout), vertexSize(_vertexSize), renderCallback(_renderCallback)
{

}

void Shader::BindTexture(CommandList& commandList, uint32_t rootParamIndex, uint32_t offset, std::shared_ptr<Texture>& texture)
{
    if (texture && texture->IsValid())
    {
        commandList.SetShaderResourceView(rootParamIndex, offset, *texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    else
    {
        commandList.SetShaderResourceView(rootParamIndex, offset, defaultSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

void Shader::BindTexture(CommandList& commandList, uint32_t rootParamIndex, uint32_t offset, Texture* texture)
{
    if (texture != nullptr && texture->IsValid())
    {
        commandList.SetShaderResourceView(rootParamIndex, offset, *texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    else
    {
        commandList.SetShaderResourceView(rootParamIndex, offset, defaultSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

std::shared_ptr<Shader> Shader::ShaderVSPS(ComPtr<ID3D12Device2> device, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, UINT vertexLayoutCount, size_t _vertexSize, std::shared_ptr<RootSignature> rootSignature, ShaderRender _renderCallback, std::wstring shaderName, D3D12_CULL_MODE cullMode, bool enableTransparency, DXGI_FORMAT rtvFormat)
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(shaderName, _vertexLayout, _vertexSize, _renderCallback);
    shader->rootSignature = rootSignature;
    shader->shaderType = ShaderType::VSPS;

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
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(rootSignature->GetRootSignatureDesc());
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob);
    ThrowBlobIfFailed(hr, errorBlob);

    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

    // D3D12_GRAPHICS_PIPELINE_STATE_DESC
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS Flags;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC Blend;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = rtvFormat;

    CD3DX12_RASTERIZER_DESC rasterizerDesc{ CD3DX12_DEFAULT() };
    rasterizerDesc.CullMode = cullMode;

    CD3DX12_BLEND_DESC blendDesc{ CD3DX12_DEFAULT() };
    if (enableTransparency)
    {
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    pipelineStateStream.pRootSignature = shader->rootSignature->GetRootSignature().Get();
    pipelineStateStream.InputLayout = { _vertexLayout, vertexLayoutCount };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;
    pipelineStateStream.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    pipelineStateStream.SampleDesc = sampleDesc;
    pipelineStateStream.Rasterizer = rasterizerDesc;
    pipelineStateStream.Blend = blendDesc;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&shader->pipelineState)));

    shader->pipelineState->SetName(shaderName.c_str());

    // Create an SRV that can be used to pad unused texture slots.
    D3D12_SHADER_RESOURCE_VIEW_DESC SRV;
    SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRV.Texture2D.MostDetailedMip = 0;
    SRV.Texture2D.MipLevels = 1;
    SRV.Texture2D.PlaneSlice = 0;
    SRV.Texture2D.ResourceMinLODClamp = 0;
    SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shader->defaultSRV = std::make_shared<ShaderResourceView>(nullptr, &SRV);

    return shader;
}

std::shared_ptr<Shader> Shader::ShaderDepthOnlyVSPS(ComPtr<ID3D12Device2> device, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, UINT vertexLayoutCount, size_t _vertexSize, std::shared_ptr<RootSignature> rootSignature, std::wstring shaderName, uint32_t depthBias)
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(shaderName, _vertexLayout, _vertexSize);
    shader->rootSignature = rootSignature;
    shader->shaderType = ShaderType::VSPS;

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
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(rootSignature->GetRootSignatureDesc());
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob);
    ThrowBlobIfFailed(hr, errorBlob);

    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

    // D3D12_GRAPHICS_PIPELINE_STATE_DESC
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS Flags;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
    } pipelineStateStream;

    // Set a null render target so that we disable color writes, optimizing performance
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 0;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_UNKNOWN;

    CD3DX12_RASTERIZER_DESC rasterizerDesc{ CD3DX12_DEFAULT() };
    rasterizerDesc.DepthBias = depthBias;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 1.0f;

    pipelineStateStream.pRootSignature = shader->rootSignature->GetRootSignature().Get();
    pipelineStateStream.InputLayout = { _vertexLayout, vertexLayoutCount };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;
    pipelineStateStream.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    pipelineStateStream.SampleDesc = sampleDesc;
    pipelineStateStream.Rasterizer = rasterizerDesc;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&shader->pipelineState)));

    shader->pipelineState->SetName(shaderName.c_str());

    // Create an SRV that can be used to pad unused texture slots.
    D3D12_SHADER_RESOURCE_VIEW_DESC SRV;
    SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRV.Texture2D.MostDetailedMip = 0;
    SRV.Texture2D.MipLevels = 1;
    SRV.Texture2D.PlaneSlice = 0;
    SRV.Texture2D.ResourceMinLODClamp = 0;
    SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shader->defaultSRV = std::make_shared<ShaderResourceView>(nullptr, &SRV);

    return shader;
}

std::shared_ptr<Shader> Shader::ShaderSkyboxVSPS(ComPtr<ID3D12Device2> device, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, UINT vertexLayoutCount, size_t _vertexSize, std::shared_ptr<RootSignature> rootSignature, ShaderRender _renderCallback, std::wstring shaderName)
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(shaderName, nullptr, 0, _renderCallback);
    shader->rootSignature = rootSignature;
    shader->shaderType = ShaderType::VSPS;

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
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(rootSignature->GetRootSignatureDesc());
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob);
    ThrowBlobIfFailed(hr, errorBlob);

    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

    // D3D12_GRAPHICS_PIPELINE_STATE_DESC
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS Flags;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC Blend;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    CD3DX12_RASTERIZER_DESC rasterizerDesc{ CD3DX12_DEFAULT() };
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    CD3DX12_BLEND_DESC blendDesc{ CD3DX12_DEFAULT() };

    CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc{CD3DX12_DEFAULT()};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    pipelineStateStream.pRootSignature = shader->rootSignature->GetRootSignature().Get();
    pipelineStateStream.InputLayout = { _vertexLayout, vertexLayoutCount };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;
    pipelineStateStream.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    pipelineStateStream.SampleDesc = sampleDesc;
    pipelineStateStream.Rasterizer = rasterizerDesc;
    pipelineStateStream.Blend = blendDesc;
    pipelineStateStream.DepthStencilState = depthStencilDesc;


    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&shader->pipelineState)));

    shader->pipelineState->SetName(shaderName.c_str());

    // Create an SRV that can be used to pad unused texture slots.
    D3D12_SHADER_RESOURCE_VIEW_DESC SRV;
    SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRV.Texture2D.MostDetailedMip = 0;
    SRV.Texture2D.MipLevels = 1;
    SRV.Texture2D.PlaneSlice = 0;
    SRV.Texture2D.ResourceMinLODClamp = 0;
    SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shader->defaultSRV = std::make_shared<ShaderResourceView>(nullptr, &SRV);

    return shader;
}

std::shared_ptr<Shader> Shader::ShaderWireframeVSPS(ComPtr<ID3D12Device2> device, D3D12_INPUT_ELEMENT_DESC* _vertexLayout, UINT vertexLayoutCount, size_t _vertexSize, std::shared_ptr<RootSignature> rootSignature, ShaderRender _renderCallback, std::wstring shaderName)
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(shaderName, _vertexLayout, _vertexSize, _renderCallback);
    shader->rootSignature = rootSignature;
    shader->shaderType = ShaderType::VSPS;

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
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(rootSignature->GetRootSignatureDesc());
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob);
    ThrowBlobIfFailed(hr, errorBlob);

    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

    // D3D12_GRAPHICS_PIPELINE_STATE_DESC
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS Flags;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC Blend;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    CD3DX12_RASTERIZER_DESC rasterizerDesc{ CD3DX12_DEFAULT() };
    rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;

    CD3DX12_BLEND_DESC blendDesc{ CD3DX12_DEFAULT() };
    pipelineStateStream.pRootSignature = shader->rootSignature->GetRootSignature().Get();
    pipelineStateStream.InputLayout = { _vertexLayout, vertexLayoutCount };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;
    pipelineStateStream.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    pipelineStateStream.SampleDesc = sampleDesc;
    pipelineStateStream.Rasterizer = rasterizerDesc;
    pipelineStateStream.Blend = blendDesc;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&shader->pipelineState)));

    shader->pipelineState->SetName(shaderName.c_str());

    // Create an SRV that can be used to pad unused texture slots.
    D3D12_SHADER_RESOURCE_VIEW_DESC SRV;
    SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRV.Texture2D.MostDetailedMip = 0;
    SRV.Texture2D.MipLevels = 1;
    SRV.Texture2D.PlaneSlice = 0;
    SRV.Texture2D.ResourceMinLODClamp = 0;
    SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shader->defaultSRV = std::make_shared<ShaderResourceView>(nullptr, &SRV);

    return shader;
}

std::shared_ptr<Shader> Shader::ShaderCS(ComPtr<ID3D12Device2> device, std::shared_ptr<RootSignature> rootSignature, std::wstring shaderName)
{
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(shaderName);
    shader->rootSignature = rootSignature;
    shader->shaderType = ShaderType::CS;

    std::wstring shaderPath = GetContentDirectoryW() + L"shaders/" + shaderName + L".hlsl";

    // Compile shaders at runtime

    ComPtr<IDxcBlob> computeShader;
    ThrowIfFailed(CompileShader(shaderPath, L"CS", L"cs_6_4", computeShader));

    if (computeShader == nullptr)
        throw std::exception("Shader did not compile");

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = rootSignature->GetRootSignature().Get();
    pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShader->GetBufferPointer(), computeShader->GetBufferSize());

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&shader->pipelineState)));

    shader->pipelineState->SetName(shaderName.c_str());

    return shader;
}
