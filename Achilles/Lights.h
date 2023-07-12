#pragma once

#include <memory>
#include <vector>
#include <d3d12.h>
#include <directxtk12/SimpleMath.h>

using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Color;

// Point light
struct PointLight
{
    // 0 bytes
    Vector4 PositionWorldSpace;
    // 16 bytes
    Vector4 PositionViewSpace;
    // 32 bytes
    Color Color;
    // 48 bytes
    float Strength;
    float Radius;
    float Distance;
    float Exponent;
    // 64 bytes

    PointLight();
};

// Spot light
struct SpotLight
{
    // 0 bytes
    PointLight Light;
    // 64 bytes
    Vector4 DirectionWorldSpace;
    // 80 bytes
    Vector4 DirectionViewSpace;
    // 96 bytes
    float SpotAngle;
    float Padding[3];
    // 112 bytes

    SpotLight();
};

// Directional light
struct DirectionalLight
{
    // 0 bytes
    Vector4 DirectionWorldSpace;
    Vector4 DirectionViewSpace;
    Color Color;
    // 64 bytes
    float Strength;
    float Padding[3];
    // 80 bytes

    DirectionalLight();
};

struct AmbientLight
{
    // 0 bytes
    Color Color;
    // 16 bytes
    float Strength;
    float Padding[3];
    // 32 bytes

    AmbientLight();
};

struct LightProperties
{
    uint32_t PointLightCount;
    uint32_t SpotLightCount;
    uint32_t DirectionalLightCount;

    LightProperties();
};

class LightData
{
public:
    std::vector<PointLight> PointLights{};
    std::vector<SpotLight> SpotLights{};
    std::vector<DirectionalLight> DirectionalLights{};
    AmbientLight AmbientLight{};

    LightProperties GetLightProperties();
};

enum class LightType
{
    None = 0,
    Point = 1,
    Spot = 2,
    Directional = 4
};
DEFINE_ENUM_FLAG_OPERATORS(LightType);