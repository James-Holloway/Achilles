struct ShadowCount
{
    uint ShadowCount;
};

struct ShadowInfo
{
    matrix ShadowMatrix;
    uint LightType;
};

ConstantBuffer<ShadowCount> ShadowCountCB : register(b0, space1);
StructuredBuffer<ShadowInfo> Shadows : register(t0, space1);

Texture2D ShadowMap0 : register(t1, space1);
Texture2D ShadowMap1 : register(t2, space1);
Texture2D ShadowMap2 : register(t3, space1);
Texture2D ShadowMap3 : register(t4, space1);
Texture2D ShadowMap4 : register(t5, space1);
Texture2D ShadowMap5 : register(t6, space1);
Texture2D ShadowMap6 : register(t7, space1);
Texture2D ShadowMap7 : register(t8, space1);

SamplerComparisonState ShadowSampler : register(s0, space1);

float CalcShadowFactor(float4 shadowPosH, Texture2D shadowMap)
{
    shadowPosH.xyz /= shadowPosH.w; // Complete projection by doing division by w
    
    // Depth in NDC space
    float depth = shadowPosH.z;
    
    uint width, height, numMips;
    shadowMap.GetDimensions(0, width, height, numMips);
    float dx = 1.0f / (float) width; // Texel size
    
    float percentLit = 0.0f;
    
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, dx), float2(0.0f, dx), float2(dx, dx)
    };

    [unroll]
    for (int i = 0; i < 9; i++)
    {
        percentLit += shadowMap.SampleCmpLevelZero(ShadowSampler, shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}