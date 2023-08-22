#include "CommonShader.hlsli"

struct PointShadowMatrices
{
    matrix MVP;
    matrix Model;
    
    float3 PointLightPosition;
    float PointLightDistance;
};

ConstantBuffer<PointShadowMatrices> ShadowMatricesCB : register(b0);
Texture2D AlphaTexture : register(t0);

SamplerState Sampler : register(s0);

struct PS_IN
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
    float4 PositionWS : TEXCOORD1;
};

PS_IN VS(CommonShaderVertex v)
{
    PS_IN o;
    o.Position = mul(ShadowMatricesCB.MVP, float4(v.Position, 1));
    o.UV = v.UV;
    o.PositionWS = mul(ShadowMatricesCB.Model, float4(v.Position, 1));
    return o;
}

void PS(PS_IN i, out float Depth : SV_Depth)
{
    float alpha = AlphaTexture.Sample(Sampler, i.UV).a;
    clip(alpha - 0.5);
    
    float lightDistance = length(i.PositionWS.xyz - ShadowMatricesCB.PointLightPosition);
    
    Depth = lightDistance / ShadowMatricesCB.PointLightDistance;
}

