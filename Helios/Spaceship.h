#pragma once
#include "Achilles/Object.h"
#include <directxtk12/SimpleMath.h>
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>
#include "Achilles/MouseData.h"
#include "Achilles/Camera.h"
#include "Achilles/MathHelpers.h"

class Spaceship : public Object
{
public:
    Spaceship();

    void SetupDrawableShip();
    void SetupCamera(int width, int height);

public:
    std::shared_ptr<Object> drawable;
    DirectX::SimpleMath::Vector3 velocity;
    DirectX::SimpleMath::Vector3 acceleration;

    std::shared_ptr<Object> playerCameraObject;
    std::shared_ptr<Camera> playerCamera;

protected:
    // Config variables
    float maxVelocity = 50.0;
    float impulse = 5.0f;

    float mouseSensitivity = 1.0f;
    float cameraBaseMoveSpeed = 4.0f;

    // Default Camera transform
    DirectX::SimpleMath::Vector3 defaultCameraPosition = DirectX::SimpleMath::Vector3(0, 4, 8);
    DirectX::SimpleMath::Quaternion defaultCameraRotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(toRad(180.0f), 0.0f, 0.0f);

public:
    void UpdatePosition(float dt);
    void ResetCamera();

public:
    virtual void OnUpdate(float dt);
    virtual void OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt);
    virtual void OnMouse(DirectX::Mouse::ButtonStateTracker mt, MouseData md, DirectX::Mouse::State state, float dt);
};

