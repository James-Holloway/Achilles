#include "ColorCommon.hlsli"

#define TONEMAPPER_NONE 0
#define TONEMAPPER_CLAMP 1
#define TONEMAPPER_EXTENDEDREINHARD 2
#define TONEMAPPER_FILMIC 3

float3 ToneMapperClamp(float3 col)
{
    return saturate(col);
}

// Reinhard by Jodie on ShaderToy
float3 ToneMapperExtendedReinhard(float3 col, float luminance)
{
    float3 tcol = col / (1.0f + col);
    return lerp(col / (1.0f + luminance), tcol, tcol);
}

// ACES Filmic Approximation by Krzysztof Narkowicz
float3 ToneMapperFilmic(float3 col)
{
    col *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((col * (a * col + b)) / (col * (c * col + d) + e));
}