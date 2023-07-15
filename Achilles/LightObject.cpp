#include "LightObject.h"
#include <DirectXMath.h>
#include <directxtk12/SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

LightObject::LightObject(std::wstring _name) : SpriteObject(_name)
{
    AddTag(ObjectTag::Light);
    SetSpriteTexture(Texture::GetCachedTexture(L"lightbulb"));
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
    lightTypes &= ~lightType;
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

void LightObject::ConstructLightPositions(std::shared_ptr<Camera> camera)
{
    Vector3 worldPos = GetWorldPosition();
    Vector4 worldPos4 = Vector4(worldPos.x, worldPos.y, worldPos.z, 1.0f);

    Matrix rotationMatrix = Matrix::CreateFromYawPitchRoll(GetWorldRotation().y + AchillesHalfPi, GetWorldRotation().x + AchillesPi + AchillesHalfPi, GetWorldRotation().z);
    Vector3 direction = rotationMatrix.Forward();
    Vector4 direction4 = Vector4(direction.x, direction.y, direction.z, 0.0f);

    if (HasLightType(LightType::Point))
    {
        pointLight.PositionWorldSpace = worldPos4;
        pointLight.PositionViewSpace = XMVector3TransformCoord(worldPos4, camera->GetView());
    }
    if (HasLightType(LightType::Spot))
    {
        spotLight.Light.PositionWorldSpace = worldPos4;
        spotLight.Light.PositionViewSpace = XMVector3TransformCoord(worldPos, camera->GetView());

        spotLight.DirectionWorldSpace = direction4;
        spotLight.DirectionViewSpace = XMVector3TransformCoord(direction, camera->GetView());
    }
    if (HasLightType(LightType::Directional))
    {
        directionalLight.DirectionWorldSpace = direction4;
        directionalLight.DirectionViewSpace = XMVector3TransformCoord(direction, camera->GetView());
    }
}

Color LightObject::GetSpriteColor()
{
    uint32_t numLights = 0;
    if (HasLightType(LightType::Point))
        numLights++;
    if (HasLightType(LightType::Spot))
        numLights++;
    if (HasLightType(LightType::Directional))
        numLights++;

    if (numLights == 0)
        return Color(1, 1, 1, 1); // default sprite color

    Color color{ 0,0,0,0 };
    if (HasLightType(LightType::Point))
        color += pointLight.Color;
    if (HasLightType(LightType::Spot))
        color += spotLight.Light.Color;
    if (HasLightType(LightType::Point))
        color += pointLight.Color;

    color.x /= numLights;
    color.y /= numLights;
    color.z /= numLights;
    color.w = 1; // set transparency to 1

    return color;
}

void LightObject::SetSpriteColor(Color color)
{
    // Do nothing
}
