#include "Spaceship.h"
#include "Achilles/shaders/BlinnPhong.h"

Spaceship::Spaceship()
{

}

void Spaceship::SetupDrawableShip()
{
    drawable = Object::CreateObjectsFromContentFile(L"spaceship.fbx", BlinnPhong::GetBlinnPhongShader(nullptr));
    AddChild(drawable);
}

void Spaceship::SetupCamera(int width, int height)
{
    playerCameraObject = std::make_shared<Object>(L"Player Camera Object");
    playerCamera = std::make_shared<Camera>(L"Player Camera", width, height);

    Camera::mainCamera = playerCamera;
    playerCamera->SetFOV(90.0f);
    playerCamera->farZ = 10000.0f; // Far farZ - 10k

    AddChild(playerCameraObject);
    playerCameraObject->SetLocalPosition(defaultCameraPosition);
    playerCameraObject->SetLocalRotation(defaultCameraRotation);
}

void Spaceship::UpdatePosition(float dt)
{
    Vector3 pos = GetWorldPosition();
    pos += velocity * dt + (0.5 * acceleration * dt * dt);
    SetWorldPosition(pos);

    // Reset acceleration
    acceleration = {};
}

void Spaceship::ResetCamera()
{
    playerCameraObject->SetLocalRotation(defaultCameraRotation);
    playerCameraObject->SetLocalPosition(defaultCameraPosition);
}

void Spaceship::OnUpdate(float dt)
{
    if (velocity.Length() > maxVelocity)
    {
        velocity.Normalize();
        velocity *= maxVelocity;
    }

    UpdatePosition(dt);

    if (playerCamera != nullptr && playerCameraObject != nullptr)
    {
        playerCamera->SetPosition(playerCameraObject->GetWorldPosition());
        playerCamera->SetRotation(playerCameraObject->GetWorldRotation().ToEuler());
    }
}

void Spaceship::OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt)
{
    if (kb.W)
        velocity += Vector3(impulse, 0, 0) * dt; // Full impulse for forward
    if (kb.S)
        velocity -= Vector3(impulse * 0.5f, 0, 0) * dt; // Half reverse impulse

    if (kb.A)
        velocity -= Vector3(0, 0, impulse * 0.25f) * dt; // Quarter impulse for strafe
    if (kb.D)
        velocity += Vector3(0, 0, impulse * 0.25f) * dt;

    if (kb.Q)
        velocity -= Vector3(0, impulse * 0.25f, 0) * dt; // Quarter impulse for strafe
    if (kb.E)
        velocity += Vector3(0, impulse * 0.25f, 0) * dt;

    Vector3 rotation = GetLocalEulerRotation();
    if (kb.Right)
        rotation.y += 0.25f * impulse * AchillesPi * dt;
    if (kb.Left)
        rotation.y -= 0.25f * impulse * AchillesPi * dt;
    if (kb.Up)
        rotation.x += 0.25f * impulse * AchillesPi * dt;
    if (kb.Down)
        rotation.x -= 0.25f * impulse * AchillesPi * dt;

    SetLocalEulerRotation(rotation);

    if (kb.Space) // Naive slow down method
    {
        if (velocity.Length() > 0.125)
            velocity -= velocity * 0.5 * dt;
        else
            velocity = Vector3(0, 0, 0);
    }
}

void Spaceship::OnMouse(DirectX::Mouse::ButtonStateTracker mt, MouseData md, DirectX::Mouse::State state, float dt)
{
    if (mt.rightButton)
    {
        float cameraRotationSpeed = toRad(45) * 0.01f * mouseSensitivity;
        playerCamera->RotateEuler(Vector3(cameraRotationSpeed * md.mouseYDelta, cameraRotationSpeed * md.mouseXDelta, 0));
        playerCameraObject->SetWorldRotation(Quaternion::CreateFromYawPitchRoll(playerCamera->GetRotation()));
    }
    else
    {
        ResetCamera();
    }

}
