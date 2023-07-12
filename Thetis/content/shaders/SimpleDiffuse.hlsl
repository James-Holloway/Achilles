struct PointLight
{
    // 0 bytes
    float4 PositionWorldSpace;
    // 16 bytes
    float4 PositionViewSpace;
    // 32 bytes
    float4 Color;
    // 48 bytes
    float Strength;
    float Radius;
    float Distance;
    float Exponent;
    // 64 bytes
};

struct SpotLight
{
    // 0 bytes
    PointLight Light;
    // 64 bytes
    float4 DirectionWorldSpace;
    // 80 bytes
    float4 DirectionViewSpace;
    // 96 bytes
    float SpotAngle;
    float3 Padding;
    // 112 bytes
};

// Directional light
struct DirectionalLight
{
    // 0 bytes
    float4 DirectionWorldSpace;
    float4 DirectionViewSpace;
    float4 Color;
    // 64 bytes
    float Strength;
    float3 Padding;
    // 80 bytes
};

struct AmbientLight
{
    // 0 bytes
    float4 Color;
    // 16 bytes
    float Strength;
    float3 Padding;
    // 32 bytes
};

struct LightProperties
{
    uint PointLightCount;
    uint SpotLightCount;
    uint DirectionalLightCount;
};

struct Matrices
{
    matrix MVP; // Model * View * Projection, used for SV_Position
    matrix MV; // Model * View, used for 
    matrix InverseMV; // Inversed then transposed model view matrix
};

struct MaterialProperties
{
    float4 Color;
};

ConstantBuffer<Matrices> MatricesCB : register(b0);
ConstantBuffer<LightProperties> LightPropertiesCB : register(b1);
ConstantBuffer<AmbientLight> AmbientLightCB : register(b2);
ConstantBuffer<MaterialProperties> MaterialPropertiesCB : register(b3);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);
StructuredBuffer<DirectionalLight> DirectionalLights : register(t2);

Texture2D DiffuseTexture : register(t3);

SamplerState TextureSampler : register(s0);

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
    float4 Position : SV_Position; // Position in screenspace
    float4 PositionVS : TEXCOORD1; // Position in viewspace
    float3 NormalVS : TEXCOORD2; // Normal in viewspace
    float3 TangentVS : TEXCOORD3; // Tangent in viewspace
    float2 UV : TEXCOORD0;
};

PS_IN VS(VS_IN v)
{
    PS_IN o;
    o.Position = mul(MatricesCB.MVP, float4(v.Position, 1));
    o.PositionVS = mul(MatricesCB.MV, float4(v.Position, 1));
    o.NormalVS = mul((float3x3) MatricesCB.InverseMV, v.Normal);
    o.TangentVS = mul((float3x3) MatricesCB.InverseMV, v.Tangent);
    o.UV = v.UV;
    
    return o;
}

float4 PS(PS_IN i) : SV_Target
{
    float4 col = DiffuseTexture.Sample(TextureSampler, i.UV);
    col *= MaterialPropertiesCB.Color;
    
    return col;
    // return i.NormalVS;
}