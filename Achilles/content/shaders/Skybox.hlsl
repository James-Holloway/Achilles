#include "CommonShader.hlsli"

struct SkyboxInfo
{
    matrix MVP;
    matrix Model;
    
    float HasTexture;
    float3 LookDirection;
    
    float PrimarySunEnable;
    float3 PrimarySunDirection;
    
    float4 PrimarySunColor;
    
    float PrimarySunSize;
    float PrimarySunShineExponent;
    float HideSunBehindHorizon;
    float Debug;

    float4 SkyColor;
    float4 UpSkyColor;
    float4 HorizonColor;
    float4 GroundColor;
};

ConstantBuffer<SkyboxInfo> SkyboxInfoCB : register(b0);
TextureCube<float4> CubemapTexture : register(t0);
SamplerState TextureSampler : register(s0);

struct PS_IN
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
    float4 Normal : TEXCOORD1;
    float3 OriginalPosition : TEXCOORD2;
};

PS_IN VS(CommonShaderVertex v)
{
    PS_IN o = (PS_IN) 0;
    o.OriginalPosition = mul(SkyboxInfoCB.Model, float4(v.Position, 1)).xyz;
    o.Position = mul(SkyboxInfoCB.MVP, float4(v.Position, 1));
    o.Normal = mul(SkyboxInfoCB.Model, float4(v.Normal, 0)).xyzw;
    o.UV = v.UV;
    return o;
}

float GetSunMask(float sunViewDot, float sunRadius)
{
    float stepRadius = 1 - sunRadius;
    return step(stepRadius, sunViewDot);
}

float4 PS(PS_IN i) : SV_Target
{
    float3 col = float3(0, 0, 0);
    
    float3 normal = -normalize(i.OriginalPosition.xyz);
    float upAmount = i.OriginalPosition.y;
    float horizonHeight = 0.25;
    
    if (SkyboxInfoCB.HasTexture)
    {
        col = CubemapTexture.Sample(TextureSampler, -normal).rgb;
    }
    else
    {
        // Sky
        col = lerp(SkyboxInfoCB.SkyColor.rgb, SkyboxInfoCB.UpSkyColor.rgb, upAmount);
        if (upAmount >= 0 && upAmount < horizonHeight)
            col = lerp(SkyboxInfoCB.HorizonColor.rgb, col, pow(upAmount * 1 / horizonHeight, 0.5));
    
        // Ground
        if (upAmount < 0)
            col = SkyboxInfoCB.GroundColor.rgb;
    }
    
    if (SkyboxInfoCB.Debug)
    {
        col = (normal + 1.0) / 2.0;
    }
    
    // Sun
    if (SkyboxInfoCB.PrimarySunEnable > 0.5)
    {   
        float sunSize = SkyboxInfoCB.PrimarySunSize / 100;
        float sunViewDot = dot(SkyboxInfoCB.PrimarySunDirection, normal);
        float sunViewDot01 = (sunViewDot + 1.0) * 0.5;
        float sunMask = GetSunMask(sunViewDot, sunSize);
        sunMask = saturate(pow(sunViewDot01 + sunSize, SkyboxInfoCB.PrimarySunShineExponent) + sunMask) * step(0.00001, sunSize);
        if (SkyboxInfoCB.HideSunBehindHorizon)
        {
            sunMask *= step(0, upAmount); // prevent sun from showing below horizon
        }
        col = lerp(col, SkyboxInfoCB.PrimarySunColor.rgb, sunMask);
    }
    
    return float4(col, 1);
}