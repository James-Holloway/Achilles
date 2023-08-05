struct Data
{
    float4 BackgroundColor;
    float2 UVOffset;
    float2 UVScale;
};

ConstantBuffer<Data> DataCB : register(b0);

Texture2D StartupTexture : register(t0);

SamplerState Sampler : register(s0);

struct PS_IN
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

PS_IN VS(in uint VertID : SV_VertexID)
{
    PS_IN o;
    o.UV = float2(uint2(VertID, VertID << 1) & 2);
    o.Position = float4(lerp(float2(-1, 1), float2(1, -1), o.UV), 0, 1);
    
    return o;
}

float4 PS(PS_IN i) : SV_Target
{
    float3 col = DataCB.BackgroundColor.rgb;
    float2 uv = (i.UV * DataCB.UVScale) + DataCB.UVOffset;
    float4 tex = StartupTexture.Sample(Sampler, uv);
    
    col = lerp(col, tex.rgb, tex.a);
    
    return float4(col, 1);

}