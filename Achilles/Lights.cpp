#include "Lights.h"

PointLight::PointLight() : PositionWorldSpace(0, 0, 0, 1), PositionViewSpace(0, 0, 0, 1), Color(1, 1, 1, 1), Strength(1), Distance(50), Radius(0.05f), Exponent(2)
{

}

// 1.0472 rad = 60 deg
SpotLight::SpotLight() : Light(), SpotAngle(1.0472f), DirectionWorldSpace(0, 0, 0, 0), DirectionViewSpace(0, 0, 0, 0), Padding{ 0 }
{

}

DirectionalLight::DirectionalLight() : DirectionWorldSpace(0, 0, 0, 0), DirectionViewSpace(0, 0, 0, 0), Color(1, 1, 1, 1), Strength(0), Padding{ 0 }
{

}

AmbientLight::AmbientLight() : Color(0, 0, 0, 1), Strength(0), Padding{ 0 }
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
