#include "Lights.h"

// Cosntant 1.0, Linear 0.007 & Quadratic 0.0002 Attenuation = 600m
PointLight::PointLight() : PositionWorldSpace(0, 0, 0, 1), PositionViewSpace(0, 0, 0, 1), Color(1, 1, 1, 1), Strength(1.0f), ConstantAttenuation(1.0f), LinearAttenuation(0.007f), QuadraticAttenuation(0.0002f), MaxDistance(500.0f), Padding{ 0 }
{

}

// 0.8727 rad = 50 deg, 1.0472 rad = 60 deg
SpotLight::SpotLight() : Light(), InnerSpotAngle(0.8727f), OuterSpotAngle(1.0472f), DirectionWorldSpace(0, 0, 0, 0), DirectionViewSpace(0, 0, 0, 0), Padding{ 0 }
{

}

DirectionalLight::DirectionalLight() : DirectionWorldSpace(0, 0, 0, 0), DirectionViewSpace(0, 0, 0, 0), Color(1, 1, 1, 1), Strength(1.0f), Padding{ 0 }
{

}

AmbientLight::AmbientLight() : Color(1, 1, 1, 1), Strength(0.125), Padding{ 0 }
{

}

LightProperties::LightProperties() : PointLightCount(0), SpotLightCount(0), DirectionalLightCount(0)
{

}

LightProperties LightData::GetLightProperties()
{
    LightProperties lightProperties = LightProperties();
    lightProperties.PointLightCount = (uint32_t)PointLights.size();
    lightProperties.SpotLightCount = (uint32_t)SpotLights.size();
    lightProperties.DirectionalLightCount = (uint32_t)DirectionalLights.size();
    return lightProperties;
}
