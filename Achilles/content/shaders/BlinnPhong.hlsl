#define MAX_SHADOW_MAPS 8
#define SHADOWS 1
#include "CommonShader.hlsli"
#include "Lighting.hlsli"

struct Matrices
{
    matrix MVP; // Model * View * Projection, used for SV_Position
    matrix Model;
    matrix View;
    matrix Projection;
    matrix InverseModel;
    matrix InverseView;
};

struct MaterialProperties
{
    // 0 bytes
    float4 Color;
    // 16 bytes
    float Opacity;
    float Diffuse;
    float Specular;
    float SpecularPower;
    // 32 bytes
    float ReceivesShadows;
    float IsTransparent;
    float2 Padding;
    // 48 bytes
};

struct PixelInfo
{
    // 0 bytes
    float3 CameraPosition;
    float ShadingType;
    // 16 bytes
};

ConstantBuffer<Matrices> MatricesCB : register(b0);
ConstantBuffer<PixelInfo> PixelInfoCB : register(b1);
ConstantBuffer<MaterialProperties> MaterialPropertiesCB : register(b2);
ConstantBuffer<LightProperties> LightPropertiesCB : register(b3);
ConstantBuffer<AmbientLight> AmbientLightCB : register(b4);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);
StructuredBuffer<DirectionalLight> DirectionalLights : register(t2);
StructuredBuffer<LightInfo> LightInfos : register(t3);

Texture2D DiffuseTexture : register(t4);

SamplerState TextureSampler : register(s0);

Texture2D ShadowMap0 : register(t0, space1);
Texture2D ShadowMap1 : register(t1, space1);
Texture2D ShadowMap2 : register(t2, space1);
Texture2D ShadowMap3 : register(t3, space1);
Texture2D ShadowMap4 : register(t4, space1);
Texture2D ShadowMap5 : register(t5, space1);
Texture2D ShadowMap6 : register(t6, space1);
Texture2D ShadowMap7 : register(t7, space1);

struct PS_IN
{
    float4 Position : SV_Position; // Position in screenspace
    float4 PositionWS : TEXCOORD1; // Position in worldspace
    float3 NormalWS : TEXCOORD2; // Normal in worldspace
    float3 TangentWS : TEXCOORD3; // Tangent in worldspace
    float2 UV : TEXCOORD0;
    
    // Shadow pos in homegenous coordinates
    float4 ShadowPosH0 : TEXCOORD4;
    float4 ShadowPosH1 : TEXCOORD5;
    float4 ShadowPosH2 : TEXCOORD6;
    float4 ShadowPosH3 : TEXCOORD7;
    float4 ShadowPosH4 : TEXCOORD8;
    float4 ShadowPosH5 : TEXCOORD9;
    float4 ShadowPosH6 : TEXCOORD10;
    float4 ShadowPosH7 : TEXCOORD11;
};

PS_IN VS(CommonShaderVertex v)
{
    PS_IN o = (PS_IN) 0;
    o.Position = mul(MatricesCB.MVP, float4(v.Position, 1));
    o.PositionWS = mul(MatricesCB.Model, float4(v.Position, 1));
    o.NormalWS = mul(MatricesCB.Model, float4(v.Normal, 0)).xyz;
    o.TangentWS = mul(MatricesCB.Model, float4(v.Tangent, 0)).xyz;
    o.UV = v.UV;
    
    int shadowCount = LightPropertiesCB.ShadowCount;
    
    if (shadowCount > 0)
        o.ShadowPosH0 = mul(o.PositionWS, LightInfos[0].ShadowMatrix);
    if (shadowCount > 1)
        o.ShadowPosH1 = mul(o.PositionWS, LightInfos[1].ShadowMatrix);
    if (shadowCount > 2)
        o.ShadowPosH2 = mul(o.PositionWS, LightInfos[2].ShadowMatrix);
    if (shadowCount > 3)
        o.ShadowPosH3 = mul(o.PositionWS, LightInfos[3].ShadowMatrix);
    if (shadowCount > 4)
        o.ShadowPosH4 = mul(o.PositionWS, LightInfos[4].ShadowMatrix);
    if (shadowCount > 5)
        o.ShadowPosH5 = mul(o.PositionWS, LightInfos[5].ShadowMatrix);
    if (shadowCount > 6)
        o.ShadowPosH6 = mul(o.PositionWS, LightInfos[6].ShadowMatrix);
    if (shadowCount > 7)
        o.ShadowPosH7 = mul(o.PositionWS, LightInfos[7].ShadowMatrix);
    
    return o;
}

LightResult DoLighting(float3 screenPos, float3 worldPos, float3 normal, float3 viewPos, float specularPower, float shadowFactors[MAX_SHADOW_MAPS])
{
    LightResult result = (LightResult) 0;
    float3 viewDir = normalize(viewPos - worldPos);
    
    LightResult lightResult;
    for (uint i = 0; i < LightPropertiesCB.LightInfoCount; i++)
    {
        LightInfo lightInfo = LightInfos[i];
        
        if (lightInfo.LightType == 1) // Point
        {
            lightResult = DoPointLighting(PointLights[lightInfo.LightIndex], worldPos, normal, viewDir, specularPower);
        }
        else if (lightInfo.LightType == 2) // Spot
        {
            lightResult = DoSpotLighting(SpotLights[lightInfo.LightIndex], worldPos, normal, viewDir, specularPower);
        }
        else if (lightInfo.LightType == 4) // Directional
        {
            lightResult = DoDirectionalLighting(DirectionalLights[lightInfo.LightIndex], normal, viewDir, specularPower);
        }
        else
        {
            lightResult.Diffuse = float3(0, 0, 0);
            lightResult.Specular = float3(0, 0, 0);
        }
        
        if (i < LightPropertiesCB.ShadowCount)
        {
            lightResult.Diffuse *= shadowFactors[i];
            lightResult.Specular *= shadowFactors[i];
        }
        
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    
    // result.Diffuse = saturate(result.Diffuse);
    // result.Specular = saturate(result.Specular);
    // result.Ambient = saturate(AmbientLightCB.Color.rgb * AmbientLightCB.Strength);
    result.Ambient = AmbientLightCB.Color.rgb * AmbientLightCB.Strength;
    
    return result;
}

float4 PS(PS_IN i) : SV_Target
{
    float4 col = DiffuseTexture.Sample(TextureSampler, i.UV);
    col *= MaterialPropertiesCB.Color;
    
    float3 normal = normalize(i.NormalWS);
    if (PixelInfoCB.ShadingType >= 0.5) // shading type of 1 means enabled
    {
        float shadowFactors[MAX_SHADOW_MAPS];
        if (MaterialPropertiesCB.ReceivesShadows > 0.5f) // receives shadows so calculate shadowFactors for the lights
        {
            float4 ShadowPos[MAX_SHADOW_MAPS] =
            {
                i.ShadowPosH0, i.ShadowPosH1, i.ShadowPosH2, i.ShadowPosH3, i.ShadowPosH4, i.ShadowPosH5, i.ShadowPosH6, i.ShadowPosH7
            };
            Texture2D ShadowMaps[MAX_SHADOW_MAPS] =
            {
                ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3, ShadowMap4, ShadowMap5, ShadowMap6, ShadowMap7
            };
            CalcShadowFactors(LightPropertiesCB.ShadowCount, ShadowPos, ShadowMaps, shadowFactors);
        }
        else // If no shadows received then populate
        {
            [unroll]
            for (int i = 0; i < MAX_SHADOW_MAPS; i++)
            {
                shadowFactors[i] = 1.0f;
            }
        }
        
        LightResult result = DoLighting(i.Position.xyz, i.PositionWS.xyz, normal, PixelInfoCB.CameraPosition, MaterialPropertiesCB.SpecularPower, shadowFactors);
        float3 diffuse = result.Diffuse * MaterialPropertiesCB.Diffuse;
        float3 specular = result.Specular * MaterialPropertiesCB.Specular;
        float3 ambient = result.Ambient;
        float3 light = diffuse + specular + ambient;
        
        col *= float4(light, 1);
        
        if (PixelInfoCB.ShadingType >= 1.5) // shading type of 2 means lighting only
        {
            col = float4(light, 1);
        }
    }
    
    return col;
}