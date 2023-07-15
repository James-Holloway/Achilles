struct SpriteProperties
{
    matrix MVP;
    float4 Color;
    float ScreenSize;
    float3 Padding;
};

ConstantBuffer<SpriteProperties> SpritePropertiesCB : register(b0);

Texture2D SpriteTexture : register(t0);

SamplerState MainSampler : register(s0);

struct VS_IN
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD0;
};

struct PS_IN
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

PS_IN VS(VS_IN v)
{
    PS_IN o;
    float3 posScaled = mul(v.Position, SpritePropertiesCB.ScreenSize);
    o.Position = mul(SpritePropertiesCB.MVP, float4(posScaled, 1));
    o.UV = v.UV;
    
    return o;
}

float4 PS(PS_IN i) : SV_TARGET
{
    return SpriteTexture.Sample(MainSampler, i.UV) * SpritePropertiesCB.Color;
}