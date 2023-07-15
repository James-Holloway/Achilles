#pragma once
#include "Common.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

class Camera
{
    friend class Achilles;
public:
    // Statics
    inline static std::shared_ptr<Camera> mainCamera{};
public:
    // Members
    std::wstring name;

    Vector3 position;
    Vector3 rotation;

    float fov = 60.0f; // vertical FOV - 60deg vertical ~= 90deg horizontal
    float nearZ = 0.1f;
    float farZ = 100.0f;
    CD3DX12_RECT scissorRect{ 0, 0, LONG_MAX, LONG_MAX };
    CD3DX12_VIEWPORT viewport;

protected:
    Matrix view;
    Matrix inverseView;
    Matrix proj;

public:
    bool dirtyViewMatrix = true;
    bool dirtyProjMatrix = true;

public:
    Camera(std::wstring _name, int width, int height);
    void UpdateViewport(int width, int height);

    Matrix GetView();
    Matrix GetProj();
    Matrix GetViewProj();
    Matrix GetInverseView();

    Vector3 GetPosition();
    Vector3 GetRotation();

    Vector3 WorldToScreen(Vector3 worldPosition, bool& visible);
    // Unfortunately returns a backface billboard. Need to cull front or flip normals
    Matrix GetBillboardMatrix(Vector3 worldPosition);

    void ConstructMatrices();
    void ConstructView();
    void ConstructProjection();

    void RotateEuler(Vector3 euler, bool unlockPitch = false, bool unlockRoll = false);
    void MoveRelative(Vector3 direction);
};