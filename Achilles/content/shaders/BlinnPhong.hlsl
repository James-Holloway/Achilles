#define SHADOWS 1
#define MAX_SPOT_SHADOW_MAPS 8
#define MAX_CASCADED_SHADOW_MAPS 4
#define MAX_NUM_CASCADES 6
#define MAX_POINT_SHADOW_MAPS 6

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
    float4 Color;
    
    float4 UVScaleOffset;

    float Opacity;
    float Diffuse;
    float Specular;
    float SpecularPower;
    
    float EmissionStrength;
    float ReceivesShadows;
    float IsTransparent;
    uint TextureFlags;
};

struct PixelInfo
{
    float3 CameraPosition;
    float ShadingType;
};

ConstantBuffer<Matrices> MatricesCB : register(b0);
ConstantBuffer<PixelInfo> PixelInfoCB : register(b1);
ConstantBuffer<MaterialProperties> MaterialPropertiesCB : register(b2);
ConstantBuffer<LightProperties> LightPropertiesCB : register(b3);
ConstantBuffer<AmbientLight> AmbientLightCB : register(b4);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);
StructuredBuffer<DirectionalLight> DirectionalLights : register(t2);
StructuredBuffer<CascadeInfo> CascadeInfos : register(t3);

Texture2D DiffuseTexture : register(t0, space1);
Texture2D NormalTexture : register(t1, space1);
Texture2D EmissionTexture : register(t2, space1);

SamplerState TextureSampler : register(s0);
SamplerState TrilinearSampler : register(s1);

Texture2D SpotShadowMap0 : register(t0, space2);
Texture2D SpotShadowMap1 : register(t1, space2);
Texture2D SpotShadowMap2 : register(t2, space2);
Texture2D SpotShadowMap3 : register(t3, space2);
Texture2D SpotShadowMap4 : register(t4, space2);
Texture2D SpotShadowMap5 : register(t5, space2);
Texture2D SpotShadowMap6 : register(t6, space2);
Texture2D SpotShadowMap7 : register(t7, space2);

Texture2D CascadedShadowMap0[MAX_NUM_CASCADES] : register(t0, space3);
Texture2D CascadedShadowMap1[MAX_NUM_CASCADES] : register(t6, space3);
Texture2D CascadedShadowMap2[MAX_NUM_CASCADES] : register(t12, space3);
Texture2D CascadedShadowMap3[MAX_NUM_CASCADES] : register(t18, space3);

TextureCube PointShadowMap0 : register(t0, space4);
TextureCube PointShadowMap1 : register(t1, space4);
TextureCube PointShadowMap2 : register(t2, space4);
TextureCube PointShadowMap3 : register(t3, space4);
TextureCube PointShadowMap4 : register(t4, space4);
TextureCube PointShadowMap5 : register(t5, space4);

struct PS_IN
{
    float4 Position : SV_Position; // Position in screenspace
    float4 PositionWS : TEXCOORD1; // Position in worldspace
    float3 NormalWS : TEXCOORD2; // Normal in worldspace
    float3 TangentWS : TEXCOORD3; // Tangent in worldspace
    float3 BitangentWS : TEXCOORD4; // Bitangent in worldspace
    float2 UV : TEXCOORD0;
    
    float Depth : TEXCOORD5;
    
    // Shadow pos in homegenous coordinates 
    float4 SpotShadowPosH0 : TEXCOORD6;
    float4 SpotShadowPosH1 : TEXCOORD7;
    float4 SpotShadowPosH2 : TEXCOORD8;
    float4 SpotShadowPosH3 : TEXCOORD9;
    float4 SpotShadowPosH4 : TEXCOORD10;
    float4 SpotShadowPosH5 : TEXCOORD11;
    float4 SpotShadowPosH6 : TEXCOORD12;
    float4 SpotShadowPosH7 : TEXCOORD13;
};

PS_IN VS(CommonShaderVertex v)
{
    PS_IN o = (PS_IN) 0;
    o.Position = mul(MatricesCB.MVP, float4(v.Position, 1));
    o.PositionWS = mul(MatricesCB.Model, float4(v.Position, 1));
    o.NormalWS = mul(MatricesCB.Model, float4(v.Normal, 0)).xyz;
    o.TangentWS = mul(MatricesCB.Model, float4(v.Tangent, 0)).xyz;
    o.BitangentWS = mul(MatricesCB.Model, float4(v.Bitangent, 0)).xyz;
    o.UV = v.UV;
    
    o.Depth = mul(MatricesCB.View, o.PositionWS).z;
    
    uint spotShadowCount = LightPropertiesCB.SpotShadowCount;
    uint cascadeShadowCount = LightPropertiesCB.CascadeShadowCount;
    
    if (spotShadowCount > 0)
        o.SpotShadowPosH0 = mul(o.PositionWS, SpotLights[0].LightInfo.ShadowMatrix);
    if (spotShadowCount > 1)
        o.SpotShadowPosH1 = mul(o.PositionWS, SpotLights[1].LightInfo.ShadowMatrix);
    if (spotShadowCount > 2)
        o.SpotShadowPosH2 = mul(o.PositionWS, SpotLights[2].LightInfo.ShadowMatrix);
    if (spotShadowCount > 3)
        o.SpotShadowPosH3 = mul(o.PositionWS, SpotLights[3].LightInfo.ShadowMatrix);
    if (spotShadowCount > 4)
        o.SpotShadowPosH4 = mul(o.PositionWS, SpotLights[4].LightInfo.ShadowMatrix);
    if (spotShadowCount > 5)
        o.SpotShadowPosH5 = mul(o.PositionWS, SpotLights[5].LightInfo.ShadowMatrix);
    if (spotShadowCount > 6)
        o.SpotShadowPosH6 = mul(o.PositionWS, SpotLights[6].LightInfo.ShadowMatrix);
    if (spotShadowCount > 7)
        o.SpotShadowPosH7 = mul(o.PositionWS, SpotLights[7].LightInfo.ShadowMatrix);
    
    return o;
}

LightResult DoLighting(float3 screenPos, float3 worldPos, float3 normal, float3 viewPos, float specularPower, float spotShadowFactors[MAX_SPOT_SHADOW_MAPS], float cascadedShadowFactors[MAX_CASCADED_SHADOW_MAPS], float pointShadowFactors[MAX_POINT_SHADOW_MAPS])
{
    LightResult result = (LightResult) 0;
    float3 viewDir = normalize(viewPos - worldPos);
    
    LightResult lightResult;
    uint i = 0;
    for (i = 0; i < LightPropertiesCB.PointLightCount; i++)
    {
        lightResult = DoPointLighting(PointLights[i], worldPos, normal, viewDir, specularPower);
        if (i < MAX_POINT_SHADOW_MAPS)
        {
            lightResult.Diffuse *= pointShadowFactors[i];
            lightResult.Specular *= pointShadowFactors[i];
        }
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    for (i = 0; i < LightPropertiesCB.SpotLightCount; i++)
    {
        lightResult = DoSpotLighting(SpotLights[i], worldPos, normal, viewDir, specularPower);
        if (i < MAX_SPOT_SHADOW_MAPS)
        {
            lightResult.Diffuse *= spotShadowFactors[i];
            lightResult.Specular *= spotShadowFactors[i];
        }
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    for (i = 0; i < LightPropertiesCB.DirectionalLightCount; i++)
    {
        lightResult = DoDirectionalLighting(DirectionalLights[i], normal, viewDir, specularPower);
        if (i < MAX_CASCADED_SHADOW_MAPS)
        {
            lightResult.Diffuse *= cascadedShadowFactors[i];
            lightResult.Specular *= cascadedShadowFactors[i];
        }
        
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    
    result.Ambient = AmbientLightCB.Color.rgb * AmbientLightCB.Strength;
    
    return result;
}

float4 PS(PS_IN i) : SV_Target
{
    float2 uv = i.UV;
    uv.xy = (uv.xy * MaterialPropertiesCB.UVScaleOffset.xy) + MaterialPropertiesCB.UVScaleOffset.zw;
    
    float4 col = DiffuseTexture.Sample(TextureSampler, uv);
    col *= MaterialPropertiesCB.Color;
    
    float3 normal = normalize(i.NormalWS);
    
    if ((MaterialPropertiesCB.TextureFlags & TEXTUREFLAGS_NORMAL) != 0)
    {
        float3 tangent = normalize(i.TangentWS);
        float3 bitangent = normalize(i.BitangentWS);
        
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        normal = DoNormalMapping(TBN, uv, NormalTexture, TrilinearSampler);
    }
    
    if (PixelInfoCB.ShadingType >= 0.5) // shading type of 1 means enabled
    {
        float spotShadowFactors[MAX_SPOT_SHADOW_MAPS];
        float cascadedShadowFactors[MAX_CASCADED_SHADOW_MAPS];
        float pointShadowFactors[MAX_POINT_SHADOW_MAPS];
        
        [branch]
        if (MaterialPropertiesCB.ReceivesShadows > 0.5f) // receives shadows so calculate shadowFactors for the lights
        {
            // Calc SpotLight shadow factors
            float4 SpotShadowPos[MAX_SPOT_SHADOW_MAPS] =
            {
                i.SpotShadowPosH0, i.SpotShadowPosH1, i.SpotShadowPosH2, i.SpotShadowPosH3, i.SpotShadowPosH4, i.SpotShadowPosH5, i.SpotShadowPosH6, i.SpotShadowPosH7
            };
            Texture2D SpotShadowMaps[MAX_SPOT_SHADOW_MAPS] =
            {
                SpotShadowMap0, SpotShadowMap1, SpotShadowMap2, SpotShadowMap3, SpotShadowMap4, SpotShadowMap5, SpotShadowMap6, SpotShadowMap7
            };
            CalcShadowSpotFactors(LightPropertiesCB.SpotShadowCount, SpotShadowPos, SpotShadowMaps, spotShadowFactors);
            
            // Calculate DirectionalLight shadow factors
            uint NumCascades[MAX_CASCADED_SHADOW_MAPS];
            for (int c = 0; c < MAX_CASCADED_SHADOW_MAPS; c++)
            {
                if (c < LightPropertiesCB.CascadeShadowCount)
                    NumCascades[c] = DirectionalLights[c].NumCascades;
                else
                    NumCascades[c] = 0;
            }
            
            CascadeInfo CInfos[MAX_CASCADED_SHADOW_MAPS * MAX_NUM_CASCADES];
            
            [unroll]
            for (uint m = 0; m < MAX_NUM_CASCADES; m++)
            {
                if (m < LightPropertiesCB.CascadeShadowCount)
                {
                    [unroll]
                    for (uint n = 0; n < MAX_CASCADED_SHADOW_MAPS; n++)
                    {
                        uint o = (m * MAX_NUM_CASCADES) + n;
                        if (n < NumCascades[m])
                            CInfos[o] = CascadeInfos[o];
                    }
                }
            }
            
            // Actually calculate the cascade shadow factors
            uint cascadeShadowCount = LightPropertiesCB.CascadeShadowCount;
            [branch]
            if (cascadeShadowCount <= 0) // No shadows
            {
                [unroll]
                for (int i = 0; i < MAX_CASCADED_SHADOW_MAPS; i++)
                {
                    cascadedShadowFactors[i] = 1.0f;
                }
            }
            else
            {
                CalcShadowCascadedFactors(cascadeShadowCount, i.PositionWS, i.Depth, CInfos, NumCascades, CascadedShadowMap0, 0, cascadedShadowFactors);
                CalcShadowCascadedFactors(cascadeShadowCount, i.PositionWS, i.Depth, CInfos, NumCascades, CascadedShadowMap1, 1, cascadedShadowFactors);
                CalcShadowCascadedFactors(cascadeShadowCount, i.PositionWS, i.Depth, CInfos, NumCascades, CascadedShadowMap2, 2, cascadedShadowFactors);
                CalcShadowCascadedFactors(cascadeShadowCount, i.PositionWS, i.Depth, CInfos, NumCascades, CascadedShadowMap3, 3, cascadedShadowFactors);
                
                if (PixelInfoCB.ShadingType >= 2.5 && PixelInfoCB.ShadingType <= 3.5) // shading type of 3 means cascade debug
                {
                    col *= CascadeDebugDraw(cascadeShadowCount, i.PositionWS, i.Depth, CInfos, NumCascades, 0);
                }
            }
            
            // Calculate PointLight shadow factors
            PointLight lights[MAX_POINT_SHADOW_MAPS];
            for (uint l = 0; l < min(LightPropertiesCB.PointLightCount, MAX_POINT_SHADOW_MAPS); l++)
            {
                lights[l] = PointLights[l];
            }
            TextureCube textures[MAX_POINT_SHADOW_MAPS] =
            {
                PointShadowMap0, PointShadowMap1, PointShadowMap2, PointShadowMap3, PointShadowMap4, PointShadowMap5
            };
            
            CalcPointShadowFactors(LightPropertiesCB.PointShadowCount, i.PositionWS.xyz, PixelInfoCB.CameraPosition.xyz, lights, textures, pointShadowFactors);

        }
        else // If no shadows received then populate
        {
            [unroll]
            for (int i = 0; i < MAX_SPOT_SHADOW_MAPS; i++)
            {
                spotShadowFactors[i] = 1.0f;
            }
            
            [unroll]
            for (int j = 0; j < MAX_CASCADED_SHADOW_MAPS; j++)
            {
                cascadedShadowFactors[j] = 1.0f;
            }
            
            [unroll]
            for (int k = 0; k < MAX_POINT_SHADOW_MAPS; k++)
            {
                pointShadowFactors[k] = 1.0f;
            }
        }
        
        LightResult result = DoLighting(i.Position.xyz, i.PositionWS.xyz, normal, PixelInfoCB.CameraPosition, MaterialPropertiesCB.SpecularPower, spotShadowFactors, cascadedShadowFactors, pointShadowFactors);
        float3 diffuse = result.Diffuse * MaterialPropertiesCB.Diffuse;
        float3 specular = result.Specular * MaterialPropertiesCB.Specular;
        float3 ambient = result.Ambient;
        float3 light = diffuse + specular + ambient;
        float4 emission = float4(0, 0, 0, 1);
        
        if ((MaterialPropertiesCB.TextureFlags & TEXTUREFLAGS_EMISSION) != 0)
        {
            emission = EmissionTexture.Sample(TextureSampler, uv);
            float emissionStrength = MaterialPropertiesCB.EmissionStrength * emission.a;
            emission.rgb *= emissionStrength * MaterialPropertiesCB.Color.rgb;
        }
        
        col = float4((light * col.rgb) + emission.rgb, col.a);
        
        if (PixelInfoCB.ShadingType >= 1.5 && PixelInfoCB.ShadingType <= 2.5) // shading type of 2 means lighting only
        {
            col = float4(light, 1);
        }
    }
    
    return col;
}