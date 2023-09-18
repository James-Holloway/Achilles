#define SHADOWS 0
#include "CommonShader.hlsli"
#include "Lighting.hlsli"

struct Matrices
{
    matrix MVP; // Model * View * Projection, used for SV_Position
    matrix VP;
    matrix Model;
    matrix View;
    matrix Projection;
    matrix InverseView;
};

struct PixelInfo
{
    float3 CameraPosition;
    float Padding;
};

struct WaterInfo
{
    float Time;
    float TimeScale;
    float WaveScale;
    float WaveHorizontalScale;
};

ConstantBuffer<Matrices> MatricesCB : register(b0);
ConstantBuffer<PixelInfo> PixelInfoCB : register(b1);
ConstantBuffer<WaterInfo> WaterInfoCB : register(b2);

ConstantBuffer<LightProperties> LightPropertiesCB : register(b3);
ConstantBuffer<AmbientLight> AmbientLightCB : register(b4);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);
StructuredBuffer<DirectionalLight> DirectionalLights : register(t2);
StructuredBuffer<CascadeInfo> CascadeInfos : register(t3);

Texture2D NoiseTexture : register(t0, space1);

SamplerState TextureSampler : register(s0);

struct PS_IN
{
    float4 Position : SV_Position; // Position in screenspace
    float4 PositionWS : TEXCOORD1; // Position in worldspace
    float3 NormalWS : TEXCOORD2; // Normal in worldspace
    float3 TangentWS : TEXCOORD3; // Tangent in worldspace
    float3 BitangentWS : TEXCOORD4; // Bitangent in worldspace
    float2 UV : TEXCOORD0;
    
    float Depth : TEXCOORD5;
};


static const float PI = 3.14159265358979;
static const float E = 2.71828;

// Different frequencies of waves
static const float wavelengths[] =
{
    31,
    9,
    17,
    5,
    19,
};

static const float offsets[] =
{
    0,
    0.15,
    0.66,
    0,
    0,
};

static const float amplitudes[] =
{
    1,
    0.5,
    0.8,
    0.1,
    0.4
};

static const float2 directions[] =
{
    float2(0.0, 1.0),
    float2(1.0, 1.0),
    float2(1.0, 0.0),
    float2(-1.0, -0.5),
    float2(0.5, 0.5),
};

static const float speeds[] = 
{
    1,
    1,
    1,
    1,
    1,
};

void WaveHeight(float2 uv, inout float3 position, out float3 normal, out float3 tangent, out float3 bitangent)
{
    normal = float3(0, 0, 0);
    tangent = float3(0, 0, 0);
    bitangent = float3(0, 0, 0);

    float time = WaterInfoCB.Time * WaterInfoCB.TimeScale;
    float waveHeight = 0.0;

    [unroll]
    for (uint i = 0; i < 5; i++)
    {
        float wavelength = wavelengths[i] * WaterInfoCB.WaveHorizontalScale;
        float amplitude = amplitudes[i] * WaterInfoCB.WaveScale;
        float frequency = (2.0 * PI) / wavelength;
        float phi = speeds[i] * frequency;
        float steepness = 0.8f;
        float2 direction = normalize(directions[i]);

        float Qi = steepness / (frequency * amplitude * 4.0);
        float term = frequency * dot(direction, uv.xy) + time * phi + offsets[i];

        float C = cos(term);
        float S = sin(term);
        position += float3(Qi * amplitude * direction.x * C, amplitude * S, Qi * amplitude * direction.y * C);
        // position += float3(0, amplitude * S, 0);

        float WA = frequency * amplitude;

        C = C / 6.0;
        tangent += float3(Qi * direction.x * direction.x * WA * S, Qi * direction.y * direction.y * WA * S, direction.y * WA * C);
        bitangent += float3(Qi * direction.x * direction.x * WA * S, Qi * direction.x * direction.y * WA * S, direction.x * WA * C);
        normal += float3(direction.x * WA * C, direction.y * WA * C, Qi * WA * S);
    }

    normal  = normalize(float3(-normal.x, -normal.y, 1.0 - normal.z));
    tangent = normalize(float3(-tangent.x, 1.0 - tangent.y, tangent.z));
    bitangent = normalize(float3(1.0 - bitangent.x, -bitangent.y, bitangent.z));
}

PS_IN VS(CommonShaderVertex v)
{
    PS_IN o = (PS_IN) 0;

    o.PositionWS = mul(MatricesCB.Model, float4(v.Position, 1));

    float3 position, normal, tangent, bitangent;

    position = o.PositionWS.xyz;

    WaveHeight(v.UV, position, normal, tangent, bitangent);

    o.Position = mul(MatricesCB.VP, float4(position, 1.0));
    // o.NormalWS = mul(MatricesCB.Model, float4(normal, 0)).xyz;
    o.NormalWS = normal;
    o.TangentWS = mul(MatricesCB.Model, float4(tangent, 0)).xyz;
    o.BitangentWS = mul(MatricesCB.Model, float4(bitangent, 0)).xyz;
    o.UV = v.UV;
    
    o.Depth = mul(MatricesCB.View, float4(position, 1.0)).z;

    return o;
}

LightResult DoLighting(float3 screenPos, float3 worldPos, float3 normal, float3 viewPos, float specularPower)
{
    LightResult result = (LightResult) 0;
    float3 viewDir = normalize(viewPos - worldPos);
    
    LightResult lightResult;
    uint i = 0;
    for (i = 0; i < LightPropertiesCB.PointLightCount; i++)
    {
        lightResult = DoPointLighting(PointLights[i], worldPos, normal, viewDir, specularPower);
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    for (i = 0; i < LightPropertiesCB.SpotLightCount; i++)
    {
        lightResult = DoSpotLighting(SpotLights[i], worldPos, normal, viewDir, specularPower);
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    for (i = 0; i < LightPropertiesCB.DirectionalLightCount; i++)
    {
        lightResult = DoDirectionalLighting(DirectionalLights[i], normal, viewDir, specularPower);
        
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    
    result.Ambient = AmbientLightCB.Color.rgb * AmbientLightCB.Strength;
    
    return result;
}

float4 PS(PS_IN i) : SV_Target
{
    float2 uv = i.UV;

    float3 col = float3(0.65, 0.76, 0.96);

    float3 normal = normalize(i.NormalWS);

    LightResult lighting = DoLighting(i.Position.xyz, i.PositionWS.xyz, normal, PixelInfoCB.CameraPosition, 32);

    float3 light = lighting.Diffuse + lighting.Specular + lighting.Ambient;

    col = light * col;

    return float4(col, 0.85);
}