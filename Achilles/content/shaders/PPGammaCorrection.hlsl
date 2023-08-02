#include "ColorCommon.hlsli"

RWTexture2D<float3> Color : register(u0);

cbuffer CB0 : register(b0)
{
    float2 g_RcpBufferDim;
    float GammaCorrection;
}

[numthreads(8, 8, 1)]
void CS(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    float2 TexCoord = (DTid.xy + 0.5) * g_RcpBufferDim;

    float3 color = Color[DTid.xy].rgb;
    float luma = RGBToLogLuminance(color);
    
    if (GammaCorrection == 2.2)
    {
        color = ApplySRGBCurve(color);
    }
    else
    {
        color = EncodeGamma(color, GammaCorrection);
    }
   
    Color[DTid.xy] = saturate(color);
}