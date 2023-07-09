#pragma once

#include "Common.h"
#include "Texture.h"

enum AttachmentPoint
{
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    DepthStencil,
    NumAttachmentPoints
};

using RenderTargetList = std::vector<std::shared_ptr<Texture>>;

class RenderTarget
{
public:
    // Create an empty render target.
    RenderTarget();

    RenderTarget(const RenderTarget& copy) = default;
    RenderTarget(RenderTarget&& copy) = default;

    RenderTarget& operator=(const RenderTarget& other) = default;
    RenderTarget& operator=(RenderTarget&& other) = default;

    // Attach a texture to the render target.
    // The texture will be copied into the texture array.
    void AttachTexture(AttachmentPoint attachmentPoint, std::shared_ptr<Texture> texture);
    const std::shared_ptr<Texture> GetTexture(AttachmentPoint attachmentPoint) const;

    // Resize all of the textures associated with the render target.
    void Resize(uint32_t width, uint32_t height);

    // Get a list of the textures attached to the render target.
    // This method is primarily used by the CommandList when binding the render target to the output merger stage of the rendering pipeline.
    RenderTargetList GetTextures() const;

    // Get the render target formats of the textures currently attached to this render target object.
    // This is needed to configure the Pipeline state object.
    D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

    // Get the format of the attached depth/stencil buffer.
    DXGI_FORMAT GetDepthStencilFormat() const;

    // Get the sample description of the render target.
    DXGI_SAMPLE_DESC GetSampleDesc() const;

    // Reset all textures
    void Reset();


private:
    RenderTargetList textures;
};