#pragma once

#include <memory>
#include <vector>
#include <d3d12.h>
#include <directxtk12/SimpleMath.h>

constexpr size_t MAX_SHADOW_MAPS = 8;

class LightObject;
class ShadowCamera;
class ShadowMap;
class Material;

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Color;

// Point light
struct PointLight
{
    // 0 bytes
    Vector4 PositionWorldSpace;
    // 16 bytes
    Color Color;
    // 32 bytes
    float Strength;
    float ConstantAttenuation;
    float LinearAttenuation;
    float QuadraticAttenuation;
    // 48 bytes
    float MaxDistance;
    float Rank;
    float Padding[2];
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
    Vector4 RotationWorldSpace;
    // 96 bytes
    float InnerSpotAngle;
    float OuterSpotAngle;
    float Padding[2];
    // 112 bytes

    SpotLight();
};

// Directional light
struct DirectionalLight
{
    // 0 bytes
    Vector4 DirectionWorldSpace;
    // 16 bytes
    Vector4 RotationWorldSpace;
    // 32 bytes
    Color Color;
    // 48 bytes
    float Strength;
    float Rank;
    float Padding[2];
    // 64 bytes

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

struct ShadowCount
{
    uint32_t ShadowCount;
};

struct ShadowInfo
{
    Matrix ShadowMatrix;
    uint32_t LightType;
};

class LightData
{
public:
    std::vector<PointLight> PointLights{};
    std::vector<SpotLight> SpotLights{};
    std::vector<DirectionalLight> DirectionalLights{};
    AmbientLight AmbientLight{};

    std::vector<std::shared_ptr<ShadowCamera>> ShadowCameras{};
    std::vector<std::shared_ptr<ShadowMap>> SortedShadowMaps{};
    std::vector<ShadowInfo> SortedShadows{};

    ShadowCount ShadowCount;
    
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