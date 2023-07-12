#include "LightObject.h"

LightObject::LightObject(std::wstring _name) : Object(_name)
{
    RemoveTags(ObjectTag::Mesh);
    AddTag(ObjectTag::Light);
}

LightObject::~LightObject()
{

}

bool LightObject::HasLightType(LightType lightType)
{
    return (size_t)(lightTypes & lightType) > 0;
}

void LightObject::AddLight(PointLight _pointLight)
{
    pointLight = _pointLight;
    lightTypes |= LightType::Point;
}

void LightObject::AddLight(SpotLight _spotLight)
{
    spotLight = _spotLight;
    lightTypes |= LightType::Spot;
}

void LightObject::AddLight(DirectionalLight _directionalLight)
{
    directionalLight = _directionalLight;
    lightTypes |= LightType::Directional;
}

void LightObject::RemoveLight(LightType lightType)
{
    lightTypes |= ~lightType;
}

PointLight& LightObject::GetPointLight()
{
    return pointLight;
}

SpotLight& LightObject::GetSpotLight()
{
    return spotLight;
}

DirectionalLight& LightObject::GetDirectionalLight()
{
    return directionalLight;
}
