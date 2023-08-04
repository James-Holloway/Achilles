#pragma once

#include "Texture.h"

class ShadowMap : public Texture
{
public:
    ShadowMap(TextureUsage _textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");
    ShadowMap(const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr, TextureUsage _textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");
    ShadowMap(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr, TextureUsage _textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");

    virtual ~ShadowMap() {};

    ShadowMap& operator=(const ShadowMap& other);
    ShadowMap& operator=(ShadowMap&& other) noexcept;

    float GetRank();
    void SetRank(float _rank);

    virtual void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1) override;

    std::shared_ptr<Texture> GetReadableDepthTexture();
    void SetReadableDepthTexture(std::shared_ptr<Texture> _readableDepthTexture);
    void CopyDepthToReadableDepthTexture(std::shared_ptr<CommandList> commandList);

    static std::shared_ptr<ShadowMap> CreateShadowMap(uint32_t width, uint32_t height);
protected:
    float rank = 0.0f;
    std::shared_ptr<Texture> readableDepthTexture;
};