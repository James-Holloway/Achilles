#pragma once

enum class TextureUsage
{
    // Treat Diffuse and Albedo textures the same
    Generic,
    Albedo = Generic,
    Diffuse = Albedo,
    // Depth Textures
    Depth,
    // Linear Textures
    Linear,
    Heightmap,
    Normalmap,
    // Texture is used as a render target
    RenderTarget,
    // Texture is used as a cubemap
    Cubemap,
    // Is sRGB
    sRGB,
};