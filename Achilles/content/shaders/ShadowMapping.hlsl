struct ShadowMatrices
{
    matrix MVP;
};

ConstantBuffer<ShadowMatrices> ShadowMatricesCB : register(b0);
Texture2D AlphaTexture : register(t0);

SamplerState Sampler : register(s0);

// CommonShaderVertex
struct VS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 Bitangent : BITANGENT;
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
    o.Position = mul(ShadowMatricesCB.MVP, float4(v.Position, 1));
    o.UV = v.UV;
    return o;
}

void PS(PS_IN i)
{
    float alpha = AlphaTexture.Sample(Sampler, i.UV).a;
    clip(alpha - 0.5);
}

