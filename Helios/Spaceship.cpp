#include "Spaceship.h"
#include "Achilles/LightObject.h"
#include "Achilles/shaders/BlinnPhong.h"

Spaceship::Spaceship()
{

}

void Spaceship::SetupDrawableShip()
{
    drawable = Object::CreateObjectsFromContentFile(L"spaceship.fbx", BlinnPhong::GetBlinnPhongShader(nullptr));
    AddChild(drawable);

    thrusterPointLight = Object::CreateLightObject(L"Spaceship Thruster Point Light");
    PointLight pointLight;
    pointLight.Light.Color = Color(0.259f, 0.735f, 0.995f);
    thrusterPointLight->AddLight(pointLight);
    AddChild(thrusterPointLight);
    thrusterPointLight->SetLocalPosition(Vector3(0, 1.75f, 4.25f));
    thrusterPointLight->SetIsShadowCaster(true);

    spotLightObject = Object::CreateLightObject(L"Spaceship Spot Light");
    SpotLight spot;
    spot.Light.MaxDistance = 500.0f;
    spotLightObject->AddLight(spot);
    AddChild(spotLightObject);
    spotLightObject->SetLocalPosition(Vector3(0.0f, 1.0f, 4.5f));
    spotLightObject->SetLocalRotation(Quaternion::CreateFromYawPitchRoll(toRad(-90.0f), 0, 0));
    spotLightObject->SetActive(false); // Disable as rotation doesn't work properly yet - engine bug (TODO)
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

    if (thrusterPointLight != nullptr)
    {
        // Increase thruster light intensity the faster you are going. Strength ranges from 0-2
        thrusterPointLight->GetPointLight().Light.Strength = velocity.Length() / (maxVelocity * 0.5f);
    }
}

void Spaceship::OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt)
{
    Vector3 newVelocity;
    if (kb.W)
        newVelocity -= Vector3(0, 0, impulse) * dt; // Full impulse for forward
    if (kb.S)
        newVelocity += Vector3(0, 0, impulse * 0.5f) * dt; // Half reverse impulse

    if (kb.D)
        newVelocity -= Vector3(impulse * 0.25f, 0, 0) * dt; // Quarter impulse for strafe
    if (kb.A)
        newVelocity += Vector3(impulse * 0.25f, 0, 0) * dt;

    if (kb.Q)
        newVelocity -= Vector3(0, impulse * 0.25f, 0) * dt; // Quarter impulse for strafe
    if (kb.E)
        newVelocity += Vector3(0, impulse * 0.25f, 0) * dt;

    velocity += Multiply(GetWorldRotation(), newVelocity);

    Quaternion rotation = GetWorldRotation();
    if (kb.Right)
        rotation = Quaternion::Concatenate(rotation, Quaternion::CreateFromYawPitchRoll(rotationImpulse * AchillesPi * dt, 0, 0));
    if (kb.Left)
        rotation = Quaternion::Concatenate(rotation, Quaternion::CreateFromYawPitchRoll(-rotationImpulse * AchillesPi * dt, 0, 0));
    float invertYFactor = invertY ? -1.0f : 1.0f;
    if (kb.Up)
        rotation = Quaternion::Concatenate(rotation, Quaternion::CreateFromYawPitchRoll(0, invertYFactor * rotationImpulse * AchillesPi * dt, 0));
    if (kb.Down)
        rotation = Quaternion::Concatenate(rotation, Quaternion::CreateFromYawPitchRoll(0, invertYFactor * -rotationImpulse * AchillesPi * dt, 0));

    rotation.Normalize();
    SetWorldRotation(rotation);

    if (kb.Space) // Naive slow down method
    {
        if (velocity.Length() > 0.125)
            velocity -= velocity * 2.0f * dt;
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

void Spaceship::OnGamePad(DirectX::GamePad::State state, float dt)
{
    Vector3 newVelocity;
    if (state.thumbSticks.leftY > 0)
        newVelocity += Vector3(0, 0, -state.thumbSticks.leftY * impulse) * dt; // Full impulse for forward
    else if (state.thumbSticks.leftY < 0)
        newVelocity += Vector3(0, 0, -state.thumbSticks.leftY * impulse * 0.5f) * dt; // Half reverse impulse

    newVelocity += Vector3(-state.thumbSticks.leftX * impulse * 0.25f, 0, 0) * dt; // Quarter impulse for strafe

    if (state.triggers.left > 0)
        newVelocity -= Vector3(0, state.triggers.left * impulse * 0.25f, 0) * dt; // Quarter impulse for strafe
    if (state.triggers.right > 0)
        newVelocity += Vector3(0, state.triggers.right * impulse * 0.25f, 0) * dt;

    velocity += Multiply(GetWorldRotation(), newVelocity);

    Quaternion rotation = GetWorldRotation();
    rotation = Quaternion::Concatenate(rotation, Quaternion::CreateFromYawPitchRoll(state.thumbSticks.rightX * rotationImpulse * AchillesPi * dt, 0, 0));
    float invertYFactor = invertY ? -1.0f : 1.0f;
    rotation = Quaternion::Concatenate(rotation, Quaternion::CreateFromYawPitchRoll(0, invertYFactor * state.thumbSticks.rightY * rotationImpulse * AchillesPi * dt, 0));

    rotation.Normalize();
    SetWorldRotation(rotation);

    if (state.buttons.a) // Naive slow down method
    {
        if (velocity.Length() > 0.125)
            velocity -= velocity * 2.0f * dt;
        else
            velocity = Vector3(0, 0, 0);
    }
}

DirectX::BoundingOrientedBox Spaceship::GetWorldBoundingBox()
{
    return drawable->GetWorldBoundingBox();
}