#pragma once

#include <memory>
#include <vector>
#include <d3d12.h>
#include <directxtk12/SimpleMath.h>

constexpr size_t MAX_SPOT_SHADOW_MAPS = 8; // Maximum number of spotlight shadows that are supported by shaders
constexpr size_t MAX_CASCADED_SHADOW_MAPS = 4; // Maximum number of cascaded shadow maps that are supported by shaders
constexpr size_t MAX_NUM_CASCADES = 6; // The maximum number of cascades/textures per cascaded shadow map

class LightObject;
class ShadowCamera;
class ShadowMap;
class Material;

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Color;

struct LightInfo
{
    Matrix ShadowMatrix;
    uint32_t IsShadowCaster;
    float Padding[3] = { 0 };
};

struct CascadeInfo
{
    Matrix CascadeMatrix;
    float DepthStart;
    float MinBorderPadding;
    float MaxBorderPadding;
    float Padding;

    CascadeInfo();
};

// Point light
struct LightCommon
{   
    Vector4 PositionWorldSpace;
    
    Color Color;
    
    float Strength;
    float ConstantAttenuation;
    float LinearAttenuation;
    float QuadraticAttenuation;
    
    float MaxDistance;
    float Rank;
    float Padding[2];


    LightCommon();
};

struct PointLight
{
    LightCommon Light;

    PointLight();
};

// Spot light
struct SpotLight
{
    LightCommon Light;
    
    LightInfo LightInfo;

    Vector4 DirectionWorldSpace;
    
    Vector4 RotationWorldSpace;
    
    float InnerSpotAngle;
    float OuterSpotAngle;
    float Padding[2];

    SpotLight();
};

// Directional light
struct DirectionalLight
{
    LightInfo LightInfo;
    
    Vector4 DirectionWorldSpace;
    
    Vector4 RotationWorldSpace;
    
    Color Color;
    
    float Strength;
    float Rank;
    uint32_t NumCascades;
    float Padding;
    

    DirectionalLight();
};

struct AmbientLight
{
    Color Color;

    float Strength;
    float Padding[3];
    

    AmbientLight();
};

struct LightProperties
{
    uint32_t PointLightCount;
    uint32_t SpotLightCount;
    uint32_t DirectionalLightCount;
    uint32_t SpotShadowCount;
    uint32_t CascadeShadowCount;
    float Padding[3] = { 0 };

    LightProperties();
};

class LightData
{
public:
    std::vector<PointLight> PointLights{};
    std::vector<SpotLight> SpotLights{};
    std::vector<DirectionalLight> DirectionalLights{};
    AmbientLight AmbientLight{};

    std::vector<std::shared_ptr<ShadowCamera>> ShadowCameras{};
    std::vector<std::shared_ptr<ShadowMap>> SortedSpotShadowMaps{};
    std::vector<std::shared_ptr<ShadowMap>> SortedCascadeShadowMaps{};
    std::vector<CascadeInfo> SortedCascadeShadowInfos{};
    
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

struct CombinedLight
{
    float Rank;
    LightObject* LightObject;
    LightType LightType;
    bool IsShadowCaster;
    union
    {
        PointLight PointLight;
        SpotLight SpotLight;
        DirectionalLight DirectionalLight;
    };
};

struct {
    bool operator()(CombinedLight& a, CombinedLight& b) const
    {
        if (a.IsShadowCaster != b.IsShadowCaster)
        {
            if (a.IsShadowCaster)
                return true;
            else
                return false;
        }
        return a.Rank > b.Rank;
    }
} CombinedLightRankSort;