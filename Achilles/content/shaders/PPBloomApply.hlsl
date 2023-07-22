#include "ColorCommon.hlsli"

Texture2D<float3> Bloom : register(t0);
Texture2D<float3> SrcColor : register(t1);

RWTexture2D<float3> DstColor : register(u0);
RWTexture2D<float> OutLuma : register(u1);

SamplerState LinearSampler : register(s0);

cbuffer CB0 : register(b0)
{
    float2 g_RcpBufferDim;
    float g_BloomStrength;
}

[numthreads(8, 8, 1)]
void CS(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    float2 TexCoord = (DTid.xy + 0.5) * g_RcpBufferDim;

    // Load LDR and bloom
    float3 ldrColor = SrcColor[DTid.xy] + g_BloomStrength * Bloom.SampleLevel(LinearSampler, TexCoord, 0);

    DstColor[DTid.xy] = ldrColor;
    OutLuma[DTid.xy] = RGBToLogLuminance(ldrColor);
}