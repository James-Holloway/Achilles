#pragma once
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
using DirectX::BoundingBox;

constexpr uint32_t ShadowCameraWidth = 1024;
constexpr uint32_t ShadowCameraNumCascades = 3; // Default to 3 cascades

class LightObject;

class ShadowCamera : public Camera
{
public:
    ShadowCamera();
    ShadowCamera(std::wstring _name, uint32_t width, uint32_t height);
    virtual ~ShadowCamera();

    void UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingBox shadowBounds, std::shared_ptr<Camera> camera, uint32_t bufferPrecision = 16);
    void UpdateMatrix(Vector3 lightPos, Quaternion lightRotation, BoundingBox shadowBounds, std::shared_ptr<Camera> camera, uint32_t bufferWidth, uint32_t bufferHeight, uint32_t bufferPrecision);

    const Matrix& GetShadowMatrix() const;

    std::shared_ptr<ShadowMap> GetShadowMap(uint32_t index = 0);
    std::shared_ptr<RenderTarget> GetShadowMapRenderTarget();

    LightType GetLightType();
    void SetLightType(LightType _lightType);

    // Set by LightObject so we can get the lightobject from just this camera
    LightObject* GetLightObject();
    void SetLightObject(LightObject* _lightObject);

    float GetRank();
    void SetRank(float rank);

    void ResizeShadowMaps(uint32_t width, uint32_t height);
    void ResizeCascades(uint32_t newCascades);
    uint32_t GetNumCascades();

    std::vector<Matrix> GetCascadeProjections();
    std::vector<Matrix> GetCascadeMatrices();
    std::vector<float> GetCascadePartitions();

protected:
    // Returns a vector of orthographic matrices, one per cascade. used when setting the directional light's projection matrix
    std::vector<Matrix> GetDirectionalLightFrustumFromSceneAndCamera(BoundingBox sceneAABB, std::shared_ptr<Camera> camera, uint32_t numCascades);

public:
    uint32_t cameraWidth = ShadowCameraWidth;
    uint32_t cameraHeight = ShadowCameraWidth;

protected:
    Matrix shadowMatrix;
    std::vector<Matrix> cascadeProjections;
    std::vector<std::shared_ptr<ShadowMap>> shadowMaps;
    std::shared_ptr<RenderTarget> shadowMapRT;
    LightType lightType = LightType::None;
    LightObject* lightObject{};

    uint32_t numCascades = ShadowCameraNumCascades;
    std::vector<float> cascadePartitions{};

    float rank = -1000.0f; // Return small rank so it becomes the last in the sorted shadow camera list

public:
    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    static const inline Matrix NDCToTextureTransform = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    };
};