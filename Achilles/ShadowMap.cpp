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

float ShadowMap::GetRank()
{
    return rank;
}

void ShadowMap::SetRank(float _rank)
{
    rank = _rank;
}

void ShadowMap::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize)
{
    Texture::Resize(width, height, depthOrArraySize);

    if (readableDepthTexture != nullptr)
    {
        readableDepthTexture->Resize(width, height, depthOrArraySize);
    }
}

std::shared_ptr<Texture> ShadowMap::GetReadableDepthTexture()
{
    return readableDepthTexture;
}

void ShadowMap::SetReadableDepthTexture(std::shared_ptr<Texture> _readableDepthTexture)
{
    readableDepthTexture = _readableDepthTexture;
}

void ShadowMap::CopyDepthToReadableDepthTexture(std::shared_ptr<CommandList> commandList)
{
    if (readableDepthTexture == nullptr)
        throw std::exception("Readable Depth Texture was not set, you probably didn't use ShadowMap::CreateShadowMap");

    commandList->CopyResource(*readableDepthTexture, *this);
    commandList->TransitionBarrier(*readableDepthTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, false);
    commandList->TransitionBarrier(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
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

    CD3DX12_RESOURCE_DESC readableTextureDesc{};
    readableTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, depthTextureDesc.Width, depthTextureDesc.Height, 1U, 1U, 1U, 0U, D3D12_RESOURCE_FLAG_NONE);
    std::shared_ptr<Texture> readableTexture = std::make_shared<Texture>(readableTextureDesc, nullptr, TextureUsage::Depth);
    readableTexture->SetName(L"Readable Depth Texture");

    shadowMap->SetReadableDepthTexture(readableTexture);

    return shadowMap;
}
