#pragma once

#include "SpriteObject.h"
#include "Lights.h"
#include "Camera.h"
#include "ShadowCamera.h"

using DirectX::SimpleMath::Color;

class LightObject : public SpriteObject
{
public:
    LightObject(std::wstring _name = Object::DefaultName);
    virtual ~LightObject();
    virtual std::shared_ptr<Object> Clone(std::shared_ptr<Object> newParent = nullptr) override;

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

    bool IsShadowCaster();
    void SetIsShadowCaster(bool _shadowCaster);
    // @param camera should be the camera the scene is being rendered from. Use nullptr to NOT update the matrix at the same time
    // @returns nullptr if the camera doesn't have the light type or if it is not a shadow caster, otherwise the relevant shadow camera
    std::shared_ptr<ShadowCamera> GetShadowCamera(LightType lightType, std::shared_ptr<Camera> camera);

protected:
    LightType lightTypes = LightType::None;
    PointLight pointLight;
    SpotLight spotLight;
    DirectionalLight directionalLight;

    bool shadowCaster = false;
    std::shared_ptr<ShadowCamera> pointShadowCamera = nullptr;
    std::shared_ptr<ShadowCamera> spotShadowCamera = nullptr;
    std::shared_ptr<ShadowCamera> directionalShadowCamera = nullptr;
};
