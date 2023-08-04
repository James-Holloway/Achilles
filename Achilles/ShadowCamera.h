#pragma once

#include "Camera.h"
#include "ShadowMap.h"
#include "RenderTarget.h"
#include "Lights.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Quaternion;
using DirectX::BoundingSphere;

constexpr uint32_t ShadowCameraWidth = 2048;
constexpr uint32_t ShadowCameraHeight = 2048;

class LightObject;

class ShadowCamera : public Camera
{
public:
    ShadowCamera();
    ShadowCamera(std::wstring _name, uint32_t width, uint32_t height);
    virtual ~ShadowCamera();

    void UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingSphere shadowBounds, uint32_t bufferPrecision = 16);
    void UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingSphere shadowBounds, uint32_t bufferWidth, uint32_t bufferHeight, uint32_t bufferPrecision);

    const Matrix& GetShadowMatrix() const;

    std::shared_ptr<ShadowMap> GetShadowMap();
    std::shared_ptr<RenderTarget> GetShadowMapRenderTarget();

    LightType GetLightType();
    void SetLightType(LightType _lightType);

    // Set by LightObject so we can get the lightobject from just this camera
    LightObject* GetLightObject();
    void SetLightObject(LightObject* _lightObject);

    float GetRank();
    void SetRank(float rank);

    void ResizeShadowMap(uint32_t width, uint32_t height);

    uint32_t cameraWidth = ShadowCameraWidth;
    uint32_t cameraHeight = ShadowCameraHeight;

protected:
    Matrix shadowMatrix;
    std::shared_ptr<ShadowMap> shadowMap;
    std::shared_ptr<RenderTarget> shadowMapRT;
    LightType lightType = LightType::None;
    LightObject* lightObject{};
};