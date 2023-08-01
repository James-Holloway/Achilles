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

std::shared_ptr<Object> LightObject::Clone(std::shared_ptr<Object> newParent)
{
    if (newParent == nullptr)
        newParent = GetParent();

    std::shared_ptr<LightObject> clone = std::make_shared<LightObject>(name + L" (Clone)");
    clone->SetParent(newParent);
    clone->SetLocalPosition(GetLocalPosition());
    clone->SetLocalRotation(GetLocalRotation());
    clone->SetLocalScale(GetLocalScale());
    for (uint32_t i = 0; i < knits.size(); i++)
    {
        clone->SetKnit(i, Knit(knits[i]));
    }
    clone->SetActive(IsActive());

    if (HasLightType(LightType::Point))
    {
        PointLight light = GetPointLight();
        clone->AddLight(light);
    }
    if (HasLightType(LightType::Spot))
    {
        SpotLight light = GetSpotLight();
        clone->AddLight(light);
    }
    if (HasLightType(LightType::Directional))
    {
        DirectionalLight light = GetDirectionalLight();
        clone->AddLight(light);
    }

    clone->SetIsShadowCaster(IsShadowCaster());

    for (auto child : GetChildren())
    {
        child->Clone(clone);
    }

    return clone;
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

    // Matrix rotationMatrix = Matrix::CreateFromYawPitchRoll(GetWorldRotation().y + AchillesHalfPi, GetWorldRotation().x + AchillesPi + AchillesHalfPi, GetWorldRotation().z);
    // Vector3 direction = rotationMatrix.Forward();
    // Vector3 rotation = GetWorldRotation().ToEuler();
    // rotation.x += AchillesPi + AchillesHalfPi;
    // rotation.y += AchillesHalfPi;
    // rotation.z = 0;
    // Quaternion quat = Quaternion::CreateFromYawPitchRoll(rotation.y, rotation.x, rotation.z);
    Matrix fix = Matrix::CreateFromYawPitchRoll(0, -AchillesHalfPi, 0);
    Quaternion quat = GetWorldRotation();
    quat *= Quaternion::CreateFromRotationMatrix(fix);
    //Vector3 direction = Multiply(quat, Vector3::Forward;
    Vector3 direction = (GetWorldMatrix() * fix).Backward(); // we need LH rather than SimpleMath's RH
    Vector4 direction4 = Vector4(direction.x, direction.y, direction.z, 0.0f);

    if (HasLightType(LightType::Point))
    {
        pointLight.PositionWorldSpace = worldPos4;
    }
    if (HasLightType(LightType::Spot))
    {
        spotLight.Light.PositionWorldSpace = worldPos4;

        spotLight.DirectionWorldSpace = direction4;
        spotLight.RotationWorldSpace = (Vector4)quat;
    }
    if (HasLightType(LightType::Directional))
    {
        directionalLight.DirectionWorldSpace = direction4;
        directionalLight.RotationWorldSpace = (Vector4)quat;
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
    if (HasLightType(LightType::Directional))
        color += directionalLight.Color;

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

bool LightObject::IsShadowCaster()
{
    return shadowCaster;
}

void LightObject::SetIsShadowCaster(bool _shadowCaster)
{
    shadowCaster = _shadowCaster;
}

std::shared_ptr<ShadowCamera> LightObject::GetShadowCamera(LightType lightType)
{
    if (!HasLightType(lightType))
        return nullptr;

    if (!IsShadowCaster())
        return nullptr;

    Vector3 shadowCenter = Vector3::Zero;
    BoundingSphere shadowBounds{ shadowCenter, 10.0f };

    switch (lightType)
    {
    case LightType::Point:
    {
        if (pointShadowCamera == nullptr)
        {
            pointShadowCamera = std::make_shared<ShadowCamera>();
            pointShadowCamera->SetLightType(LightType::Point);
        }
        pointShadowCamera->SetLightObject(this);
        pointShadowCamera->UpdateMatrix(GetWorldPosition(), GetWorldRotation(), shadowBounds);
        return pointShadowCamera;
    }
    break;
    case LightType::Spot:
    {
        if (spotShadowCamera == nullptr)
        {
            spotShadowCamera = std::make_shared<ShadowCamera>();
            spotShadowCamera->SetLightType(LightType::Spot);
        }
        spotShadowCamera->SetLightObject(this);
        spotShadowCamera->UpdateMatrix(GetWorldPosition(), (Quaternion)spotLight.RotationWorldSpace, shadowBounds);
        return spotShadowCamera;
    }
    break;
    case LightType::Directional:
    {
        if (directionalShadowCamera == nullptr)
        {
            directionalShadowCamera = std::make_shared<ShadowCamera>();
            directionalShadowCamera->SetLightType(LightType::Directional);
        }
        directionalShadowCamera->SetLightObject(this);

        Vector3 lightDir = (Vector3)directionalLight.DirectionWorldSpace;
        Vector3 lightPos = -2.0f * shadowBounds.Radius * lightDir;

        directionalShadowCamera->UpdateMatrix(lightPos, (Quaternion)directionalLight.RotationWorldSpace, shadowBounds);
        return directionalShadowCamera;
    }
    break;
    }
    return nullptr;
}
