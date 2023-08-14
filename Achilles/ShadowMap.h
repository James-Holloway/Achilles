#pragma once

#include "Texture.h"

class ShadowMap : public Texture
{
public:
    ShadowMap(TextureUsage _textureUsage = TextureUsage::Depth, const std::wstring& name = L"");
    ShadowMap(const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr, TextureUsage _textureUsage = TextureUsage::Depth, const std::wstring& name = L"");
    ShadowMap(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr, TextureUsage _textureUsage = TextureUsage::Depth, const std::wstring& name = L"");

    virtual ~ShadowMap() {};

    ShadowMap& operator=(const ShadowMap& other);
    ShadowMap& operator=(ShadowMap&& other) noexcept;

    virtual void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1) override;

    static std::shared_ptr<ShadowMap> CreateShadowMap(uint32_t width, uint32_t height);
    static D3D12_SHADER_RESOURCE_VIEW_DESC GetShadowMapR32SRV();
};