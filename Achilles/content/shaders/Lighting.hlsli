#ifndef MAX_SPOT_SHADOW_MAPS
#define MAX_SPOT_SHADOW_MAPS 8
#endif
#ifndef MAX_CASCADED_SHADOW_MAPS
#define MAX_CASCADED_SHADOW_MAPS 4
#endif
#ifndef MAX_NUM_CASCADES
#define MAX_NUM_CASCADES 6
#endif

#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2
#define LIGHT_TYPE_DIRECTIONAL 4

// Structs
struct LightInfo
{
    matrix ShadowMatrix;
    uint IsShadowCaster;
    float3 Padding;
};

struct LightCommon
{
    float4 PositionWorldSpace;
    
    float4 Color;
    
    float Strength;
    float ConstantAttenuation;
    float LinearAttenuation;
    float QuadraticAttenuation;
    
    float MaxDistance;
    float Rank;
    float2 Padding;
};

struct PointLight
{
    LightCommon Light;
};

struct SpotLight
{
    LightCommon Light;
    
    LightInfo LightInfo;
    
    float4 DirectionWorldSpace;
    
    float4 RotationWorldSpace;
    
    float InnerSpotAngle;
    float OuterSpotAngle;
    float2 Padding;
};

struct DirectionalLight
{
    LightInfo LightInfo;
    
    float4 DirectionWorldSpace;
    
    float4 RotationWorldSpace;
    
    float4 Color;
    
    float Strength;
    float Rank;
    uint NumCascades;
    float Padding;
    
};

struct AmbientLight
{
    float4 Color;
    
    float Strength;
    float3 Padding;
};

struct LightProperties
{
    uint PointLightCount;
    uint SpotLightCount;
    uint DirectionalLightCount;
    uint SpotShadowCount;
    
    uint CascadeShadowCount;
    float3 Padding;
};

struct LightResult
{
    float3 Diffuse;
    float3 Specular;
    float3 Ambient;
};

struct CascadeInfo
{
    matrix CascadeMatrix;
    float DepthStart;
    float MinBorderPadding;
    float MaxBorderPadding;
    float Padding;
};

// Lighting
float DoDiffuse(float3 normal, float3 lightDir)
{
    return max(0, dot(normal, lightDir));
}

float DoSpecular(float3 viewDir, float3 normal, float3 lightDir, float specularPower)
{
    float3 halfwayDir = normalize(lightDir + viewDir);
    return pow(max(0, dot(normal, halfwayDir)), specularPower);
}

float DoAttenuation(float cnst, float linr, float quad, float dist)
{
    return 1.0f / (cnst + linr * dist + quad * dist * dist);
}

float DoSpotCone(float3 spotDir, float3 lightDir, float innerSpotAngle, float outerSpotAngle)
{
    float theta = dot(lightDir, normalize(-spotDir));
    float epsilon = max(outerSpotAngle - innerSpotAngle, 0.0f);
    return saturate((theta - cos(outerSpotAngle)) / epsilon);
}

LightResult DoPointLighting(PointLight light, float3 worldPos, float3 normal, float3 viewDir, float specularPower)
{
    LightResult lightResult = (LightResult) 0;
    float3 lightDir = (light.Light.PositionWorldSpace.xyz - worldPos);
    float distance = length(lightDir);
    if (distance > light.Light.MaxDistance)
    {
        return lightResult;
    }
    lightDir = lightDir / distance;
    
    float attenuation = DoAttenuation(light.Light.ConstantAttenuation, light.Light.LinearAttenuation, light.Light.QuadraticAttenuation, distance);
    
    lightResult.Diffuse = DoDiffuse(normal, lightDir) * light.Light.Color.rgb * light.Light.Strength * attenuation;
    lightResult.Specular = DoSpecular(viewDir, normal, lightDir, specularPower) * light.Light.Color.rgb * light.Light.Strength * attenuation;
    return lightResult;
}

LightResult DoSpotLighting(SpotLight light, float3 worldPos, float3 normal, float3 viewDir, float specularPower)
{
    LightResult lightResult = (LightResult) 0;
    float3 lightDir = (light.Light.PositionWorldSpace.xyz - worldPos);
    float distance = length(lightDir);
    if (distance > light.Light.MaxDistance)
    {
        return lightResult;
    }
    lightDir = lightDir / distance;
    
    float spotIntensity = DoSpotCone(light.DirectionWorldSpace.xyz, lightDir, light.InnerSpotAngle, light.OuterSpotAngle);
    
    float attenuation = DoAttenuation(light.Light.ConstantAttenuation, light.Light.LinearAttenuation, light.Light.QuadraticAttenuation, distance);
    
    lightResult.Diffuse = DoDiffuse(normal, lightDir) * light.Light.Color.rgb * light.Light.Strength * spotIntensity * attenuation;
    lightResult.Specular = DoSpecular(viewDir, normal, lightDir, specularPower) * light.Light.Color.rgb * light.Light.Strength * spotIntensity * attenuation;
    return lightResult;
}

LightResult DoDirectionalLighting(DirectionalLight light, float3 normal, float3 viewDir, float specularPower)
{
    LightResult lightResult = (LightResult) 0;
    float3 lightDir = normalize(-light.DirectionWorldSpace.xyz);
    
    lightResult.Diffuse = DoDiffuse(normal, lightDir) * light.Color.rgb * light.Strength;
    lightResult.Specular = DoSpecular(viewDir, normal, lightDir, specularPower) * light.Color.rgb * light.Strength;
    return lightResult;
}

// Shadows
#if SHADOWS
SamplerComparisonState ShadowSampler : register(s0, space1);

float CalcShadowFactor(float4 shadowPos, Texture2D shadowMap)
{
    // Depth in NDC space
    float depth = shadowPos.z;
    
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
        percentLit += shadowMap.SampleCmpLevelZero(ShadowSampler, shadowPos.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}

float CalcShadowFactorDivision(float4 shadowPosH, Texture2D shadowMap)
{
    shadowPosH.xyz /= shadowPosH.w; // Complete projection by doing division by w
    return CalcShadowFactor(shadowPosH, shadowMap);
}

void CalcShadowSpotFactors(in uint shadowCount, in float4 ShadowPos[MAX_SPOT_SHADOW_MAPS], in Texture2D ShadowMaps[MAX_SPOT_SHADOW_MAPS], out float shadowFactors[MAX_SPOT_SHADOW_MAPS])
{
    if (shadowCount <= 0) // No shadows
    {
        [unroll]
        for (int i = 0; i < MAX_SPOT_SHADOW_MAPS; i++)
        {
            shadowFactors[i] = 1.0f;
        }
        return;
    }
    
    [unroll]
    for (int s = 0; s < MAX_SPOT_SHADOW_MAPS; s++)
    {
        if (s < shadowCount)
            shadowFactors[s] = CalcShadowFactorDivision(ShadowPos[s], ShadowMaps[s]);
        else
            shadowFactors[s] = 1.0f;
    }
    
    return;
}

void CalcShadowCascadedFactors(in uint shadowCount, in float4 PositionWS, in float PixelDepth, in CascadeInfo CascadeInfos[MAX_CASCADED_SHADOW_MAPS * MAX_NUM_CASCADES], in uint NumCascades[MAX_CASCADED_SHADOW_MAPS], in Texture2D ShadowMaps[MAX_NUM_CASCADES], in uint MapOffset, out float shadowFactors[MAX_CASCADED_SHADOW_MAPS])
{
    if (MapOffset < shadowCount)
    {
        // Select cascade by interval
        /*
        uint cascade = 0;
        uint nc = NumCascades[MapOffset];
        bool cascadeFound = false;
        CascadeInfo ci;
        for (int c = nc - 1; c >= 0; c--)
        {
            ci = CascadeInfos[(MapOffset * MAX_NUM_CASCADES) + c];
            cascade = c;
            if (ci.DepthStart < PixelDepth)
            {
                cascadeFound = true;
                break;
            }
        }
        //*/
        // Select cascade by map
        uint cascade = 0;
        uint nc = NumCascades[MapOffset];
        bool cascadeFound = false;
        float4 shadowPos = float4(0, 0, 0, 0);
        CascadeInfo ci;
        for (int c = 0; c < min(nc, MAX_NUM_CASCADES); c++)
        {
            ci = CascadeInfos[(MapOffset * MAX_NUM_CASCADES) + c];
            shadowPos = mul(PositionWS, ci.CascadeMatrix);
            // TODO add min/max border instead of 0/1
            if ((min(shadowPos.x, shadowPos.y) > ci.MinBorderPadding) && (max(shadowPos.x, shadowPos.y) < ci.MaxBorderPadding))
            {
                cascade = c;
                cascadeFound = true;
                break;
            }
        }
        
        // float4 shadowPos = mul(PositionWS, ci.CascadeMatrix);
        if (cascadeFound)
        {
            shadowFactors[MapOffset] = CalcShadowFactorDivision(shadowPos, ShadowMaps[cascade]);
        }
        else
            shadowFactors[MapOffset] = 1.0f;
    }
    else
    {
        shadowFactors[MapOffset] = 1.0f;
    }
}

static const float4 CascadeColorMultipliers[8] =
{
    float4(1.5f, 0.0f, 0.0f, 1.0f), // red
    float4(0.0f, 1.5f, 0.0f, 1.0f), // green
    float4(0.0f, 0.0f, 5.5f, 1.0f), // blue
    float4(1.5f, 0.0f, 5.5f, 1.0f), // pink
    float4(1.5f, 1.5f, 0.0f, 1.0f), // yellow
    float4(0.0f, 1.0f, 5.5f, 1.0f), // light blue
    float4(0.5f, 3.5f, 0.8f, 1.0f), // mint
    float4(1.0f, 1.0f, 1.0f, 1.0f), // white
};

float4 CascadeDebugDraw(in uint shadowCount, in float4 PositionWS, in float PixelDepth, in CascadeInfo CascadeInfos[MAX_CASCADED_SHADOW_MAPS * MAX_NUM_CASCADES], in uint NumCascades[MAX_CASCADED_SHADOW_MAPS], in uint MapOffset)
{
    if (MapOffset < shadowCount)
    {
        // Select cascade by interval
        /*
        uint cascade = 0;
        uint nc = NumCascades[MapOffset];
        CascadeInfo ci;
        for (int c = nc - 1; c >= 0; c--)
        {
            ci = CascadeInfos[(MapOffset * MAX_NUM_CASCADES) + c];
            cascade = c;
            if (ci.DepthStart < PixelDepth)
            {
                return CascadeColorMultipliers[cascade];
            }
        }
        //*/
        
        // Select cascade by map
        uint cascade = 0;
        uint nc = NumCascades[MapOffset];
        CascadeInfo ci;
        
        for (int c = 0; c < min(nc, MAX_NUM_CASCADES); c++)
        {
            ci = CascadeInfos[(MapOffset * MAX_NUM_CASCADES) + c];
            float4 shadowPos = mul(PositionWS, ci.CascadeMatrix);
            // TODO add min/max border instead of 0/1
            if ((min(shadowPos.x, shadowPos.y) > ci.MinBorderPadding) && (max(shadowPos.x, shadowPos.y) < ci.MaxBorderPadding))
            {
                cascade = c;
                return CascadeColorMultipliers[cascade];
            }
        }
        
    }
    return float4(0.25, 0.25, 0.25, 1.0f); // grey
}

#endif

// Normals
float3 ExpandNormal(float3 n)
{
    return n * 2.0f - 1.0f;
}

float3 DoNormalMapping(float3x3 TBN, float2 uv, Texture2D tex, SamplerState s)
{
    float3 N = tex.Sample(s, uv).xyz;
    
    N = ExpandNormal(N);
    N = mul(N, TBN);
    return normalize(N);
}