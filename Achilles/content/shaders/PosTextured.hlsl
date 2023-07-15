struct _PosTexturedCB0
{
    float4x4 mvp;
};

ConstantBuffer<_PosTexturedCB0> PosTexturedCB0 : register(b0);

Texture2D MainTexture : register(t0);

SamplerState MainSampler : register(s0);

struct VS_IN
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
};

struct PS_IN
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD;
};

PS_IN VS(VS_IN v)
{
    PS_IN o;
    float4x4 mvp = PosTexturedCB0.mvp;
    o.Position = mul(mvp, float4(v.Position, 1.0f));
    o.UV = v.UV;
    
    return o;
}

float4 PS(PS_IN i) : SV_Target
{
    return MainTexture.Sample(MainSampler, i.UV);
}