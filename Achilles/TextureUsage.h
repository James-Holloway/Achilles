#pragma once

enum class TextureUsage
{
    // Treat Diffuse and Albedo textures the same
    Albedo,
    Diffuse = Albedo,
    // Treat height and depth textures the same
    Heightmap,
    Depth = Heightmap,
    Linear = Depth,
    Normalmap,
    // Texture is used as a render target
    RenderTarget,
};