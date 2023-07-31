#include "Texture.h"
#include "Application.h"
#include "ResourceStateTracker.h"
#include "CommandList.h"

Texture::Texture(TextureUsage _textureUsage, const std::wstring& name) : Resource(name) , textureUsage(_textureUsage) {}

Texture::Texture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue, TextureUsage _textureUsage, const std::wstring& name) : Resource(resourceDesc, clearValue, name), textureUsage(_textureUsage)
{
    CreateViews();
}

Texture::Texture(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* _clearValue, TextureUsage _textureUsage, const std::wstring& name) : Resource(resource, _clearValue, name), textureUsage(_textureUsage)
{
    CreateViews();
}

Texture::Texture(const Texture& copy) : Resource(copy)
{
    textureUsage = copy.textureUsage;
    CreateViews();
}

Texture::Texture(Texture&& copy) noexcept : Resource(copy)
{
    textureUsage = copy.textureUsage;
    CreateViews();
}

Texture& Texture::operator=(const Texture& other)
{
    Resource::operator=(other);

    CreateViews();

    return *this;
}
Texture& Texture::operator=(Texture&& other) noexcept
{
    Resource::operator=(other);

    CreateViews();

    return *this;
}

Texture::~Texture() {}

void Texture::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize)
{
    // Resource can't be resized if it was never created in the first place.
    if (d3d12Resource)
    {
        ResourceStateTracker::RemoveGlobalResourceState(d3d12Resource.Get());

        CD3DX12_RESOURCE_DESC resDesc(d3d12Resource->GetDesc());

        resDesc.Width = std::max(width, 1u);
        resDesc.Height = std::max(height, 1u);
        resDesc.DepthOrArraySize = depthOrArraySize;

        auto device = Application::GetD3D12Device();

        CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COMMON, d3d12ClearValue.get(), IID_PPV_ARGS(&d3d12Resource)));

        // Retain the name of the resource if one was already specified.
        d3d12Resource->SetName(resourceName.c_str());

        ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

        CreateViews();
    }
}

bool Texture::GetSize(float& width, float& height)
{
    width = 0;
    height = 0;
    if (d3d12Resource)
    {
        CD3DX12_RESOURCE_DESC resDesc(d3d12Resource->GetDesc());
        width = resDesc.Width;
        height = resDesc.Height;
        return true;
    }
    return false;
}

// Get a UAV description that matches the resource description.
D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(const D3D12_RESOURCE_DESC& resDesc, UINT mipSlice, UINT arraySlice = 0, UINT planeSlice = 0)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = resDesc.Format;

    switch (resDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (resDesc.DepthOrArraySize > 1)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uavDesc.Texture1DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
            uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
            uavDesc.Texture1DArray.MipSlice = mipSlice;
        }
        else
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = mipSlice;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (resDesc.DepthOrArraySize > 1)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
            uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
            uavDesc.Texture2DArray.PlaneSlice = planeSlice;
            uavDesc.Texture2DArray.MipSlice = mipSlice;
        }
        else
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.PlaneSlice = planeSlice;
            uavDesc.Texture2D.MipSlice = mipSlice;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.WSize = resDesc.DepthOrArraySize - arraySlice;
        uavDesc.Texture3D.FirstWSlice = arraySlice;
        uavDesc.Texture3D.MipSlice = mipSlice;
        break;
    default:
        throw std::exception("Invalid resource dimension.");
    }

    return uavDesc;
}

void Texture::CreateViews()
{
    if (d3d12Resource)
    {
        auto device = Application::GetD3D12Device();

        CD3DX12_RESOURCE_DESC desc(d3d12Resource->GetDesc());

        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport;
        formatSupport.Format = desc.Format;
        ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));

        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && CheckRTVSupport(formatSupport.Support1))
        {
            renderTargetView = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            device->CreateRenderTargetView(d3d12Resource.Get(), nullptr, renderTargetView.GetDescriptorHandle());
        }
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && CheckDSVSupport(formatSupport.Support1))
        {
            depthStencilView = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            device->CreateDepthStencilView(d3d12Resource.Get(), nullptr, depthStencilView.GetDescriptorHandle());
        }
    }

    std::lock_guard<std::mutex> lock(shaderResourceViewsMutex);
    std::lock_guard<std::mutex> guard(unorderedAccessViewsMutex);

    // SRVs and UAVs will be created as needed.
    shaderResourceViews.clear();
    unorderedAccessViews.clear();
}

DescriptorAllocation Texture::CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    auto device = Application::GetD3D12Device();
    auto srv = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device->CreateShaderResourceView(d3d12Resource.Get(), srvDesc, srv.GetDescriptorHandle());

    return srv;
}

DescriptorAllocation Texture::CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    auto device = Application::GetD3D12Device();
    auto uav = Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device->CreateUnorderedAccessView(d3d12Resource.Get(), nullptr, uavDesc, uav.GetDescriptorHandle());

    return uav;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    std::size_t hash = 0;
    if (srvDesc)
    {
        hash = std::hash<D3D12_SHADER_RESOURCE_VIEW_DESC>{}(*srvDesc);
    }

    std::lock_guard<std::mutex> lock(shaderResourceViewsMutex);

    auto iter = shaderResourceViews.find(hash);
    if (iter == shaderResourceViews.end())
    {
        auto srv = CreateShaderResourceView(srvDesc);
        iter = shaderResourceViews.insert({ hash, std::move(srv) }).first;
    }

    return iter->second.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    std::size_t hash = 0;
    if (uavDesc)
    {
        hash = std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>{}(*uavDesc);
    }

    std::lock_guard<std::mutex> guard(unorderedAccessViewsMutex);

    auto iter = unorderedAccessViews.find(hash);
    if (iter == unorderedAccessViews.end())
    {
        auto uav = CreateUnorderedAccessView(uavDesc);
        iter = unorderedAccessViews.insert({ hash, std::move(uav) }).first;
    }

    return iter->second.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRenderTargetView() const
{
    return renderTargetView.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDepthStencilView() const
{
    return depthStencilView.GetDescriptorHandle();
}

bool Texture::IsUAVCompatibleFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        // case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SINT:
        return true;
    default:
        return false;
    }
}

bool Texture::IsSRGBFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return true;
    default:
        return false;
    }
}

bool Texture::IsBGRFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return true;
    default:
        return false;
    }
}

bool Texture::IsDepthFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D16_UNORM:
        return true;
    default:
        return false;
    }
}

DXGI_FORMAT Texture::GetTypelessFormat(DXGI_FORMAT format)
{
    DXGI_FORMAT typelessFormat = format;

    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
        break;
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
        break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
        break;
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
        break;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
        typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
        break;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        break;
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
        typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        typelessFormat = DXGI_FORMAT_R32_TYPELESS;
        break;
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
        break;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
        typelessFormat = DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
        typelessFormat = DXGI_FORMAT_R8_TYPELESS;
        break;
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
        break;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
        break;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
        break;
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
        break;
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
        typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
        break;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
        break;
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
        typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
        break;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
        break;
    }

    return typelessFormat;
}

void Texture::AddCachedTexture(std::wstring contentName, std::shared_ptr<Texture> texture)
{
    textureCache[contentName] = texture;
}

std::shared_ptr<Texture> Texture::AddCachedTextureFromContent(std::shared_ptr<CommandList> commandList, std::wstring contentName, TextureUsage textureUsage)
{
    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    commandList->LoadTextureFromContent(*texture, contentName, textureUsage);
    AddCachedTexture(contentName, texture);
    return texture;
}

std::shared_ptr<Texture> Texture::GetCachedTexture(std::wstring contentName)
{
    auto iter = textureCache.find(contentName);
    if (iter != textureCache.end())
    {
        return iter->second;
    }
    return nullptr;
}

std::vector<std::wstring> Texture::GetCachedTextureNames()
{
    std::vector<std::wstring> names{};
    for (auto pair : textureCache)
    {
        names.push_back(pair.first);
    }
    return names;
}

std::map<std::wstring, std::shared_ptr<Texture>>& Texture::GetTextureCache()
{
    return textureCache;
}
