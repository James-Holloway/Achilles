#include "CommonShader.hlsli"

struct Data
{
    matrix MVP; // Model * View * Projection, used for SV_Position
    matrix Model;
    float4 Color;
};

ConstantBuffer<Data> DataCB: register(b0);

struct PS_IN
{
    float4 Position : SV_Position; // Position in screenspace
    float4 PositionWS : TEXCOORD1; // Position in worldspace
    float3 NormalWS : TEXCOORD2; // Normal in worldspace
    float2 UV : TEXCOORD0;
};

PS_IN VS(CommonShaderVertex v)
{
    PS_IN o = (PS_IN) 0;
    o.Position = mul(DataCB.MVP, float4(v.Position, 1));
    o.PositionWS = mul(DataCB.Model, float4(v.Position, 1));
    o.NormalWS = mul(DataCB.Model, float4(v.Normal, 0)).xyz;
    o.UV = v.UV;
    
    return o;
}

float4 PS(PS_IN i) : SV_Target
{
    return float4(DataCB.Color.rgb, 1);
}