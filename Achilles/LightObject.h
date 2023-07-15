#pragma once

#include "SpriteObject.h"
#include "Lights.h"
#include "Camera.h"

using DirectX::SimpleMath::Color;

class LightObject : public SpriteObject
{
public:
    LightObject(std::wstring _name = Object::DefaultName);
    virtual ~LightObject();

    bool HasLightType(LightType lightType);

    void AddLight(PointLight _pointLight);
    void AddLight(SpotLight _spotLight);
    void AddLight(DirectionalLight _directionalLight);

    void RemoveLight(LightType lightType);

    PointLight& GetPointLight();
    SpotLight& GetSpotLight();
    DirectionalLight& GetDirectionalLight();

    void ConstructLightPositions(std::shared_ptr<Camera> camera);

    Color GetSpriteColor() override;
    void SetSpriteColor(Color color) override;

protected:
    LightType lightTypes = LightType::None;
    PointLight pointLight;
    SpotLight spotLight;
    DirectionalLight directionalLight;
};
