struct PointLight
{
    // 0 bytes
    float4 PositionWorldSpace;
    // 16 bytes
    float4 Color;
    // 32 bytes
    float Strength;
    float ConstantAttenuation;
    float LinearAttenuation;
    float QuadraticAttenuation;
    // 48 bytes
    float MaxDistance;
    float Rank;
    float2 Padding;
    // 64 bytes
};

struct SpotLight
{
    // 0 bytes
    PointLight Light;
    // 64 bytes
    float4 DirectionWorldSpace;
    // 80 bytes
    float4 RotationWorldSpace;
    // 96 bytes
    float InnerSpotAngle;
    float OuterSpotAngle;
    float2 Padding;
    // 112 bytes
};

// Directional light
struct DirectionalLight
{
    // 0 bytes
    float4 DirectionWorldSpace;
    // 16 bytes
    float4 RotationWorldSpace;
    // 32 bytes
    float4 Color;
    // 48 bytes
    float Strength;
    float Rank;
    float2 Padding;
    // 64 bytes
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

struct ShadowCount
{
    uint ShadowCount;
};

struct ShadowInfo
{
    matrix ShadowMatrix;
};

ConstantBuffer<Matrices> MatricesCB : register(b0);
ConstantBuffer<PixelInfo> PixelInfoCB : register(b1);
ConstantBuffer<MaterialProperties> MaterialPropertiesCB : register(b2);
ConstantBuffer<LightProperties> LightPropertiesCB : register(b3);
ConstantBuffer<AmbientLight> AmbientLightCB : register(b4);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);
StructuredBuffer<DirectionalLight> DirectionalLights : register(t2);

Texture2D DiffuseTexture : register(t3);

SamplerState TextureSampler : register(s0);

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

PS_IN VS(VS_IN v)
{
    PS_IN o = (PS_IN) 0;
    o.Position = mul(MatricesCB.MVP, float4(v.Position, 1));
    o.PositionWS = mul(MatricesCB.Model, float4(v.Position, 1));
    o.NormalWS = mul(MatricesCB.Model, float4(v.Normal, 0)).xyz;
    o.TangentWS = mul(MatricesCB.Model, float4(v.Tangent, 0)).xyz;
    o.UV = v.UV;
    
    int shadowCount = ShadowCountCB.ShadowCount;
    
    if (shadowCount > 0)
        o.ShadowPosH0 = mul(o.PositionWS, Shadows[0].ShadowMatrix);
    if (shadowCount > 1)
        o.ShadowPosH1 = mul(o.PositionWS, Shadows[1].ShadowMatrix);
    if (shadowCount > 2)
        o.ShadowPosH2 = mul(o.PositionWS, Shadows[2].ShadowMatrix);
    if (shadowCount > 3)
        o.ShadowPosH3 = mul(o.PositionWS, Shadows[3].ShadowMatrix);
    if (shadowCount > 4)
        o.ShadowPosH4 = mul(o.PositionWS, Shadows[4].ShadowMatrix);
    if (shadowCount > 5)
        o.ShadowPosH5 = mul(o.PositionWS, Shadows[5].ShadowMatrix);
    if (shadowCount > 6)
        o.ShadowPosH6 = mul(o.PositionWS, Shadows[6].ShadowMatrix);
    if (shadowCount > 7)
        o.ShadowPosH7 = mul(o.PositionWS, Shadows[7].ShadowMatrix);
    
    return o;
}

struct LightResult
{
    float3 Diffuse;
    float3 Specular;
    float3 Ambient;
};

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
    float epsilon = innerSpotAngle - outerSpotAngle;
    return clamp(1.0f - ((theta - outerSpotAngle) / epsilon), 0.0f, 1.0f);
}

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

float CalcShadowFactors(PS_IN i)
{
    float shadowFactor = 0.0f;
    
    int shadowCount = ShadowCountCB.ShadowCount;
    
    if (shadowCount <= 0)
        return shadowFactor;
    
    // Only calculate further shadows if we're not in shadow
    if (shadowCount > 0)
        shadowFactor += CalcShadowFactor(i.ShadowPosH0, ShadowMap0);
    if (shadowCount > 1 && shadowFactor < 1)
        shadowFactor += CalcShadowFactor(i.ShadowPosH1, ShadowMap1);
    if (shadowCount > 2 && shadowFactor < 1)
        shadowFactor += CalcShadowFactor(i.ShadowPosH2, ShadowMap2);
    if (shadowCount > 3 && shadowFactor < 1)
        shadowFactor += CalcShadowFactor(i.ShadowPosH3, ShadowMap3);
    if (shadowCount > 4 && shadowFactor < 1)
        shadowFactor += CalcShadowFactor(i.ShadowPosH4, ShadowMap4);
    if (shadowCount > 5 && shadowFactor < 1)
        shadowFactor += CalcShadowFactor(i.ShadowPosH5, ShadowMap5);
    if (shadowCount > 6 && shadowFactor < 1)
        shadowFactor += CalcShadowFactor(i.ShadowPosH6, ShadowMap6);
    if (shadowCount > 7 && shadowFactor < 1)
        shadowFactor += CalcShadowFactor(i.ShadowPosH7, ShadowMap7);
    
    return saturate(shadowFactor);
}

LightResult DoPointLighting(PointLight light, float3 worldPos, float3 normal, float3 viewDir, float specularPower)
{
    LightResult lightResult = (LightResult) 0;
    float3 lightDir = (light.PositionWorldSpace.xyz - worldPos);
    float distance = length(lightDir);
    if (distance > light.MaxDistance)
    {
        return lightResult;
    }
    lightDir = lightDir / distance;
    
    float attenuation = DoAttenuation(light.ConstantAttenuation, light.LinearAttenuation, light.QuadraticAttenuation, distance);
    
    lightResult.Diffuse = DoDiffuse(normal, lightDir) * light.Color.rgb * light.Strength * attenuation;
    lightResult.Specular = DoSpecular(viewDir, normal, lightDir, specularPower) * light.Color.rgb * light.Strength * attenuation;
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

LightResult DoLighting(float3 screenPos, float3 worldPos, float3 normal, float3 viewPos, float specularPower)
{
    LightResult result = (LightResult) 0;
    float3 viewDir = normalize(viewPos - worldPos);
    
    uint i = 0;
    
    for (i = 0; i < LightPropertiesCB.PointLightCount; i++)
    {
        LightResult lightResult = DoPointLighting(PointLights[i], worldPos, normal, viewDir, specularPower);
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    
    for (i = 0; i < LightPropertiesCB.SpotLightCount; i++)
    {
        LightResult lightResult = DoSpotLighting(SpotLights[i], worldPos, normal, viewDir, specularPower);
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    
    for (i = 0; i < LightPropertiesCB.DirectionalLightCount; i++)
    {
        LightResult lightResult = DoDirectionalLighting(DirectionalLights[i], normal, viewDir, specularPower);
        result.Diffuse += lightResult.Diffuse;
        result.Specular += lightResult.Specular;
    }
    
    result.Diffuse = saturate(result.Diffuse);
    result.Specular = saturate(result.Specular);
    result.Ambient = saturate(AmbientLightCB.Color.rgb * AmbientLightCB.Strength);
    
    return result;
}

float4 PS(PS_IN i) : SV_Target
{
    float4 col = DiffuseTexture.Sample(TextureSampler, i.UV);
    col *= MaterialPropertiesCB.Color;
    
    float3 normal = normalize(i.NormalWS);
    if (PixelInfoCB.ShadingType >= 0.5) // shading type of 1 means enabled
    {
        LightResult result = DoLighting(i.Position.xyz, i.PositionWS.xyz, normal, PixelInfoCB.CameraPosition, MaterialPropertiesCB.SpecularPower);
        float3 diffuse = result.Diffuse * MaterialPropertiesCB.Diffuse;
        float3 specular = result.Specular * MaterialPropertiesCB.Specular;
        float3 ambient = result.Ambient;
        float shadowFactor = 1;
        
        if (MaterialPropertiesCB.ReceivesShadows > 0.5f)
        {
            shadowFactor = CalcShadowFactors(i);
        }
        
        float3 light = (float3(diffuse + specular) * shadowFactor) + ambient;
        
        col *= float4(light, 1);
        
        if (PixelInfoCB.ShadingType >= 1.5) // shading type of 2 means lighting only
        {
            col = float4(light, 1);
        }
    }
    
    return col;
}