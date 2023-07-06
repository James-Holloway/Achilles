struct _PosColCB0
{
    float4x4 mvp;
};

ConstantBuffer<_PosColCB0> PosColCB0 : register(b0);

struct VS_IN
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct PS_IN
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

PS_IN VS(VS_IN v)
{
    PS_IN o;
    float4x4 mvp = PosColCB0.mvp;
    o.Position = mul(mvp, float4(v.Position, 1.0f));
    o.Color = v.Color;
    
    return o;
}

float4 PS(PS_IN i) : SV_Target
{
    return i.Color;
}