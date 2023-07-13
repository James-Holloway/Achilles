#include "LightObject.h"
#include <DirectXMath.h>
#include <directxtk12/SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

LightObject::LightObject(std::wstring _name) : Object(_name)
{
    SetTags(ObjectTag::Light);
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

    Matrix rotationMatrix = Matrix::CreateFromYawPitchRoll(GetWorldRotation());
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
