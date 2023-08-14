#include "Lights.h"

CascadeInfo::CascadeInfo() : CascadeMatrix(Matrix::Identity), DepthStart(0), Padding{ 0 }
{

}


// Cosntant 1.0, Linear 0.007 & Quadratic 0.0002 Attenuation = 600m
LightCommon::LightCommon() : PositionWorldSpace(0, 0, 0, 1), Color(1, 1, 1, 1), Strength(1.0f), ConstantAttenuation(1.0f), LinearAttenuation(0.007f), QuadraticAttenuation(0.0002f), MaxDistance(500.0f), Rank(1.0f), Padding{ 0 }
{

}

PointLight::PointLight()
{
    Light.Rank = 1.0f;
}

// 0.8727 rad = 50 deg, 1.0472 rad = 60 deg
SpotLight::SpotLight() : Light(), InnerSpotAngle(0.8727f), OuterSpotAngle(1.0472f), DirectionWorldSpace(0, 0, 0, 0), RotationWorldSpace(0, 0, 0, 0), Padding{ 0 }
{
    Light.Rank = 5.0f;
}

DirectionalLight::DirectionalLight() : DirectionWorldSpace(0, 0, 0, 0), RotationWorldSpace(0, 0, 0, 0), Color(1, 1, 1, 1), Strength(1.0f), Rank(10.0f), NumCascades(1), Padding{ 0 }
{

}

AmbientLight::AmbientLight() : Color(1, 1, 1, 1), Strength(0.05f), Padding{ 0 }
{

}

LightProperties::LightProperties() : PointLightCount(0), SpotLightCount(0), DirectionalLightCount(0), SpotShadowCount(0), CascadeShadowCount(0)
{

}

LightProperties LightData::GetLightProperties()
{
    LightProperties lightProperties = LightProperties();
    lightProperties.PointLightCount = (uint32_t)PointLights.size();
    lightProperties.SpotLightCount = (uint32_t)SpotLights.size();
    lightProperties.DirectionalLightCount = (uint32_t)DirectionalLights.size();
    lightProperties.SpotShadowCount = (uint32_t)SortedSpotShadowMaps.size();
    lightProperties.CascadeShadowCount = (uint32_t)SortedCascadeShadowMaps.size();

    return lightProperties;
}
