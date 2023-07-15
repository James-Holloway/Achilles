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
    float ConstantAttenuation;
    float LinearAttenuation;
    float QuadraticAttenuation;
    // 64 bytes
    float MaxDistance;
    float3 Padding;
    // 80 bytes
};

struct SpotLight
{
    // 0 bytes
    PointLight Light;
    // 80 bytes
    float4 DirectionWorldSpace;
    // 96 bytes
    float4 DirectionViewSpace;
    // 112 bytes
    float InnerSpotAngle;
    float OuterSpotAngle;
    float2 Padding;
    // 128 bytes
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
};

struct PixelInfo
{
    // 0 bytes
    float3 CameraPosition;
    float ShadingType;
    // 16 bytes
};

ConstantBuffer<Matrices> MatricesCB : register(b0);
ConstantBuffer<PixelInfo> PixelInfoCB: register(b1);
ConstantBuffer<MaterialProperties> MaterialPropertiesCB : register(b2);
ConstantBuffer<LightProperties> LightPropertiesCB : register(b3);
ConstantBuffer<AmbientLight> AmbientLightCB : register(b4);

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
    float4 PositionWS : TEXCOORD1; // Position in worldspace
    float3 NormalWS : TEXCOORD2; // Normal in worldspace
    float3 TangentWS : TEXCOORD3; // Tangent in worldspace
    float2 UV : TEXCOORD0;
};

PS_IN VS(VS_IN v)
{
    PS_IN o;
    o.Position = mul(MatricesCB.MVP, float4(v.Position, 1));
    o.PositionWS = mul(MatricesCB.Model, float4(v.Position, 1));
    o.NormalWS = mul(MatricesCB.InverseModel, float4(v.Normal, 0)).xyz;
    o.TangentWS = mul(MatricesCB.InverseModel, float4(v.Tangent, 0)).xyz;
    o.UV = v.UV;
    
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
        float4 light = float4(diffuse + specular + ambient, 1);
        
        col *= light;
        
        if (PixelInfoCB.ShadingType >= 1.5) // shading type of 2 means lighting only
        {
            col = light;
        }
    }
    
    return col;
}