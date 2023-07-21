#pragma once

#include "Common.h"
#include "Resource.h"
#include "DescriptorAllocation.h"
#include "TextureUsage.h"

class CommandList;

using Microsoft::WRL::ComPtr;

class Texture : public Resource
{
public:
    explicit Texture(TextureUsage _textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");
    explicit Texture(const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr, TextureUsage _textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");
    explicit Texture(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr, TextureUsage _textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");

    Texture(const Texture& copy);
    Texture(Texture&& copy) noexcept;

    Texture& operator=(const Texture& other);
    Texture& operator=(Texture&& other) noexcept;

    virtual ~Texture();

    TextureUsage GetTextureUsage() const
    {
        return textureUsage;
    }

    void SetTextureUsage(TextureUsage _textureUsage)
    {
        textureUsage = textureUsage;
    }

    // Resize the texture.
    void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

    // Get the size of the current texture resource. Returns whether the resource was valid
    bool GetSize(float& width, float& height);

    // Create SRV and UAVs for the resource
    virtual void CreateViews();

    // Get the SRV for a resource
    // @param dxgiFormat The required format of the resource. When accessing a depth-stencil buffer as a shader resource view, the format will be different
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

    // Get the UAV for a (sub)resource
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

    // Get the RTV for the texture
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

    // Get the DSV for the texture
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    static bool CheckSRVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
    {
        return ((formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0 || (formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) != 0);
    }

    static bool CheckRTVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
    {
        return ((formatSupport & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0);
    }

    static bool CheckUAVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
    {
        return ((formatSupport & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0);
    }

    static bool CheckDSVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
    {
        return ((formatSupport & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0);
    }

    static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
    static bool IsSRGBFormat(DXGI_FORMAT format);
    static bool IsBGRFormat(DXGI_FORMAT format);
    static bool IsDepthFormat(DXGI_FORMAT format);

    // Return a typeless format from the given format.
    static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);

protected:
    DescriptorAllocation CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const;
    DescriptorAllocation CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const;

    mutable std::unordered_map<size_t, DescriptorAllocation> shaderResourceViews;
    mutable std::unordered_map<size_t, DescriptorAllocation> unorderedAccessViews;

    mutable std::mutex shaderResourceViewsMutex;
    mutable std::mutex unorderedAccessViewsMutex;

    DescriptorAllocation renderTargetView;
    DescriptorAllocation depthStencilView;

    TextureUsage textureUsage;

protected:
    inline static std::map<std::wstring, std::shared_ptr<Texture>> textureCache{};

public:
    static void AddCachedTexture(std::wstring contentName, std::shared_ptr<Texture> texture);
    static std::shared_ptr<Texture> AddCachedTextureFromContent(std::shared_ptr<CommandList> commandList, std::wstring contentName, TextureUsage textureUsage = TextureUsage::Albedo);
    static std::shared_ptr<Texture> GetCachedTexture(std::wstring contentName);
    static std::vector<std::wstring> GetCachedTextureNames();
    static std::map<std::wstring, std::shared_ptr<Texture>>& GetTextureCache();
};