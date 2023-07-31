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