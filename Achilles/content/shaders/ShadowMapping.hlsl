struct ShadowMatrices
{
    matrix MVP;
};

ConstantBuffer<ShadowMatrices> ShadowMatricesCB : register(b0);

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
};

PS_IN VS(VS_IN v)
{
    PS_IN o;
    o.Position = mul(ShadowMatricesCB.MVP, float4(v.Position, 1));
    return o;
}

void PS(PS_IN i)
{
    
}

