#pragma once

#include <string>
#include <memory>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <directxtk12/SimpleMath.h>

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

class ShadowCamera;

class Camera
{
    friend class Achilles;
public:
    // Statics
    inline static std::shared_ptr<Camera> mainCamera{};
    inline static std::shared_ptr<ShadowCamera> debugShadowCamera{};
public:
    // Members
    std::wstring name;

    float fov = 60.0f; // vertical FOV - 60deg vertical ~= 90deg horizontal
    float nearZ = 0.1f;
    float farZ = 100.0f;
    CD3DX12_RECT scissorRect{ 0, 0, LONG_MAX, LONG_MAX };
    CD3DX12_VIEWPORT viewport;
    float orthographicSize = 1.0f;

protected:
    Vector3 position;
    Vector3 rotation;

    Matrix view;
    Matrix inverseView;
    Matrix proj;

    bool orthographic = false;

    bool dirtyViewMatrix = true;
    bool dirtyProjMatrix = true;

public:
    Camera(std::wstring _name, int width, int height);
    virtual ~Camera();
    void UpdateViewport(int width, int height);

    virtual Matrix GetView();
    virtual Matrix GetProj();
    virtual Matrix GetViewProj();
    virtual Matrix GetInverseView();

    Vector3 GetPosition();
    Vector3 GetRotation();

    // FOV is vertical in degrees
    float GetFOV();
    bool IsOrthographic();
    bool IsPerspective();

    void SetProj(Matrix _proj);
    void SetView(Matrix _view);

    void SetPosition(Vector3 _position);
    void SetRotation(Vector3 _rotation);

    // FOV is vertical in degrees
    void SetFOV(float _fov);
    void SetOrthographic(bool _orthographic);
    void SetPerspective(bool _perspective);


    Vector3 WorldToScreen(Vector3 worldPosition, bool& visible);
    // Unfortunately returns a backface billboard. Need to cull front or flip normals
    Matrix GetBillboardMatrix(Vector3 worldPosition);

    void ConstructMatrices();
    void ConstructView();
    void ConstructProjection();

    void RotateEuler(Vector3 euler, bool unlockPitch = false, bool unlockRoll = false);
    void MoveRelative(Vector3 direction);
};