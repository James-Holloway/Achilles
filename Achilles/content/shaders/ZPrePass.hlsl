#include "CommonShader.hlsli"

struct ShadowMatrices
{
    matrix MVP;
};

ConstantBuffer<ShadowMatrices> ShadowMatricesCB : register(b0);

struct PS_IN
{
    float4 Position : SV_Position;
};

PS_IN VS(CommonShaderVertex v)
{
    PS_IN o;
    o.Position = mul(ShadowMatricesCB.MVP, float4(v.Position, 1));
    return o;
}

void PS(PS_IN i)
{
}

