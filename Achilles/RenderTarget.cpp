#include "RenderTarget.h"

RenderTarget::RenderTarget() : textures(AttachmentPoint::NumAttachmentPoints) {}

// Attach a texture to the render target
// The texture will be copied into the texture array
void RenderTarget::AttachTexture(AttachmentPoint attachmentPoint, const std::shared_ptr<Texture> texture)
{
    textures[attachmentPoint] = texture;
}

const std::shared_ptr<Texture> RenderTarget::GetTexture(AttachmentPoint attachmentPoint) const
{
    return textures[attachmentPoint];
}

// Resize all of the textures associated with the render target.
void RenderTarget::Resize(uint32_t width, uint32_t height)
{
    for (auto& texture : textures)
    {
        if (texture)
            texture->Resize(width, height);
    }
}

// Get a list of the textures attached to the render target.
// This method is primarily used by the CommandList when binding the render target to the output merger stage of the rendering pipeline
RenderTargetList RenderTarget::GetTextures() const
{
    return textures;
}

D3D12_RT_FORMAT_ARRAY RenderTarget::GetRenderTargetFormats() const
{
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};

    for (int i = AttachmentPoint::Color0; i <= AttachmentPoint::Color7; ++i)
    {
        const std::shared_ptr<Texture> texture = textures[i];
        if (texture && texture->IsValid())
        {
            rtvFormats.RTFormats[rtvFormats.NumRenderTargets++] = texture->GetD3D12ResourceDesc().Format;
        }
    }

    return rtvFormats;
}

DXGI_FORMAT RenderTarget::GetDepthStencilFormat() const
{
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
    std::shared_ptr<Texture> depthStencilTexture = textures[AttachmentPoint::DepthStencil];
    if (depthStencilTexture && depthStencilTexture->IsValid())
    {
        dsvFormat = depthStencilTexture->GetD3D12ResourceDesc().Format;
    }

    return dsvFormat;
}

DXGI_SAMPLE_DESC RenderTarget::GetSampleDesc() const
{
    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
    for (int i = AttachmentPoint::Color0; i <= AttachmentPoint::Color7; ++i)
    {
        auto texture = textures[i];
        if (texture)
        {
            sampleDesc = texture->GetD3D12ResourceDesc().SampleDesc;
            break;
        }
    }

    return sampleDesc;
}

void RenderTarget::Reset()
{
    textures = RenderTargetList(AttachmentPoint::NumAttachmentPoints);
}
