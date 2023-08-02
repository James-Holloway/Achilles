#include "ToneMappers.hlsli"

Texture2D<float3> SrcColor : register(t0);
RWTexture2D<float3> DstColor : register(u0);

cbuffer CB0 : register(b0)
{
    float2 g_RcpBufferDim;
    uint g_ToneMapper;
}

[numthreads(8, 8, 1)]
void CS(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    float2 TexCoord = (DTid.xy + 0.5) * g_RcpBufferDim;

    float3 color = SrcColor[DTid.xy].rgb;
    float luminance = RGBToLogLuminance(color);
    
    switch (g_ToneMapper)
    {
        case TONEMAPPER_NONE:
            break;
        case TONEMAPPER_CLAMP:
            color = ToneMapperClamp(color);
            break;
        case TONEMAPPER_EXTENDEDREINHARD:
            color = ToneMapperExtendedReinhard(color, luminance);
            break;
        case TONEMAPPER_FILMIC:
            color = ToneMapperFilmic(color);
            break;
    }
    
    DstColor[DTid.xy] = saturate(color);
}