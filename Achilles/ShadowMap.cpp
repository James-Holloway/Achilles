#include "ShadowMap.h"
#include "Application.h"
#include "ResourceStateTracker.h"
#include "CommandList.h"

ShadowMap::ShadowMap(TextureUsage _textureUsage, const std::wstring& name) : Texture(_textureUsage, name)
{

}

ShadowMap::ShadowMap(const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue, TextureUsage _textureUsage, const std::wstring& name) : Texture(_resourceDesc, _clearValue, _textureUsage, name)
{

}

ShadowMap::ShadowMap(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* _clearValue, TextureUsage _textureUsage, const std::wstring& name) : Texture(resource, _clearValue, _textureUsage, name)
{

}

ShadowMap& ShadowMap::operator=(const ShadowMap& other)
{
    Texture::operator=(other);

    return *this;
}

ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept
{
    Texture::operator=(other);

    return *this;
}

void ShadowMap::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize)
{
    Texture::Resize(width, height, depthOrArraySize);
}

std::shared_ptr<ShadowMap> ShadowMap::CreateShadowMap(uint32_t width, uint32_t height)
{
    auto device = Application::GetD3D12Device();
    CD3DX12_RESOURCE_DESC depthTextureDesc{};

    depthTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, (UINT64)width, (UINT)height, 1U, 1U, 1U, 0U, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE optClear{};
    optClear.Format = depthTextureDesc.Format;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = (uint8_t)0U;

    std::shared_ptr<ShadowMap> shadowMap = std::make_shared<ShadowMap>(depthTextureDesc, &optClear, TextureUsage::Depth);
    shadowMap->SetName(L"ShadowMap");

    shadowMap->d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(optClear);

    return shadowMap;
}

D3D12_SHADER_RESOURCE_VIEW_DESC ShadowMap::GetShadowMapR32SRV()
{
    // Use a srvDesc to convert D32 to R32
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    return srvDesc;
}
