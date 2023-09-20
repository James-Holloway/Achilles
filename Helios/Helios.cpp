#include "Helios.h"
#include "Achilles/shaders/BlinnPhong.h"

Helios::Helios(std::wstring _name, uint32_t width, uint32_t height) : Achilles(_name, width, height)
{
}

void Helios::CollideObjects()
{
    std::vector<std::shared_ptr<Object>> objects = GetEveryActiveObject();
    for (std::shared_ptr<Object> object1 : objects)
    {
        for (std::shared_ptr<Object> object2 : objects)
        {
            if (object1 == object2)
                continue;

            if (std::shared_ptr<Spaceship> ship = std::dynamic_pointer_cast<Spaceship>(object1))
            {
                if (std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(object2))
                {
                    DirectX::BoundingOrientedBox shipOBB = ship->GetWorldBoundingBox();
                    DirectX::BoundingSphere planetBS = planet->GetBoundingSphere();

                    // If the ship is entering/intersecting a planet
                    if (planetBS.Contains(shipOBB) != DirectX::ContainmentType::DISJOINT)
                    {
                        Vector3 diff = planet->GetWorldPosition() - ship->GetWorldPosition();
                        if (ship->velocity.Dot(diff) > 0) // disallow moving further into the planet
                        {
                            ship->velocity = -ship->velocity * 0.25f; // bounce the ship backwards a bit
                        }
                    }
                }
            }
        }
    }
}

void Helios::OnUpdate(float deltaTime)
{
    if (playerShip != nullptr)
    {
        playerShip->OnUpdate(deltaTime);
    }
    CollideObjects();
}

void Helios::OnPostRender(float deltaTime)
{
    // As we have moving shadowed point lights (from spaceship thruster), we want to render shadows every frame;
    SetRenderShadowsNextFrame();

    if (showDebug)
    {
        if (ImGui::Begin("Debug", &showDebug, ImGuiWindowFlags_NoResize))
        {
            ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetWindowSize(ImVec2(400, 300), ImGuiCond_Always);

            ImGui::Text("FPS: %.2f", lastFPS);

            if (playerShip != nullptr)
            {
                Vector3 position = playerShip->GetWorldPosition();
                if (ImGui::InputFloat3("Ship Position", &position.x))
                {
                    playerShip->SetWorldPosition(position);
                }
                Vector3 rotation = EulerToDegrees(playerShip->GetWorldRotation().ToEuler());
                if (ImGui::InputFloat3("Ship Rotation", &rotation.x))
                {
                    playerShip->SetWorldRotation(Quaternion::CreateFromYawPitchRoll(EulerToRadians(rotation)));
                }

                Vector3 eulerRotation = playerShip->GetLocalEulerRotation();
                if (ImGui::InputFloat3("Ship Euler Rotation", &eulerRotation.x))
                {
                    playerShip->SetLocalEulerRotation(eulerRotation);
                }

                ImGui::InputFloat3("Ship Velocity", &playerShip->velocity.x);

                float speed = playerShip->velocity.Length();
                ImGui::BeginDisabled(true);
                ImGui::InputFloat("Ship Speed", &speed);
                ImGui::EndDisabled();

                ImGui::Checkbox("Invert ship Y", &playerShip->invertY);
            }
        }
        ImGui::End();
    }
}

void Helios::OnResize(int newWidth, int newHeight)
{
    if (playerShip != nullptr && playerShip->playerCamera != nullptr)
        playerShip->playerCamera->UpdateViewport(newWidth, newHeight);
}

void Helios::LoadContent()
{
    ACHILLES_IF_DESTROYING_RETURN();

    doZPrePass = false;

    std::shared_ptr<CommandQueue> commandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

    std::shared_ptr<CommandQueue> computeQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    std::shared_ptr<CommandList> computeList = computeQueue->GetCommandList();

    // Load Shaders
    std::shared_ptr<Shader> blinnPhongShader = BlinnPhong::GetBlinnPhongShader(device);

    // Create Player + Player Spaceship
    playerShip = std::make_shared<Spaceship>();
    playerShip->SetupCamera(clientWidth, clientHeight);
    playerShip->SetupDrawableShip();
    playerShip->SetWorldPosition(Vector3(0, 0, 10));

    AddObjectToScene(playerShip);

    // Setup light
    sun = Object::CreateLightObject(L"Sun", nullptr);
    DirectionalLight sunDirLight;
    sunDirLight.Color = Color(0.9f, 0.9f, 0.7f);
    sun->AddLight(sunDirLight);
    sun->SetLocalRotation(Quaternion::CreateFromYawPitchRoll(toRad(-90.0f), toRad(45.0f), 0.0f));
    AddObjectToScene(sun);

    lightData.AmbientLight.Strength = 0.075f;

    ACHILLES_IF_DESTROYING_RETURN();

    // Create planets
    std::shared_ptr<Planet> dustyPlanet = Planet::CreatePlanet(commandList, L"Dusty", Vector3(-500, -500, -500), 100.0f);
    dustyPlanet->SetupDrawableNormalMap(commandList);
    AddObjectToScene(dustyPlanet);

    std::shared_ptr<Planet> blueMoonPlanet = Planet::CreatePlanet(commandList, L"Blue Moon", Vector3(500, 1000, 0), 50.0f);
    AddObjectToScene(blueMoonPlanet);

    std::shared_ptr<Planet> icyPlanet = Planet::CreatePlanet(commandList, L"Icy", Vector3(1000, 0, 150), 250.0f);
    icyPlanet->SetupDrawableNormalMap(commandList);
    AddObjectToScene(icyPlanet);

    std::shared_ptr<Planet> purplePlanet = Planet::CreatePlanet(commandList, L"Purple", Vector3(0, -1000, 0), 175.0f);
    purplePlanet->SetupDrawableNormalMap(commandList);
    AddObjectToScene(purplePlanet);

    std::shared_ptr<Planet> rockyPlanet = Planet::CreatePlanet(commandList, L"Rocky", Vector3(0, 500, 750), 100.0f);
    AddObjectToScene(rockyPlanet);

    ACHILLES_IF_DESTROYING_RETURN();

    // Load Skymap
    std::shared_ptr<Texture> skymapPano = std::make_shared<Texture>(TextureUsage::Linear, L"Outdoor HDRI 064 Pano");
    commandList->LoadTextureFromContent(*skymapPano, L"Skymap", TextureUsage::Linear);
    std::shared_ptr<Texture> skymapCubemap = CreateCubemap(1024, 1024);
    skymapCubemap->SetName(L"Outdoor HDRI 64 Cubemap");
    computeList->PanoToCubemap(*skymapCubemap, *skymapPano);
    // Set the skybox's cubemap texture
    skydome->GetMaterial().SetTexture(L"Cubemap", skymapCubemap);
    skydome->GetMaterial().SetFloat(L"PrimarySunSize", 0.05f);
    skydome->GetMaterial().SetFloat(L"PrimarySunShineExponent", 256.0f);
    Texture::AddCachedTexture(L"Skymap", skymapCubemap);

    // Create post processing class
    postProcessing = std::make_shared<PostProcessing>((float)clientWidth, (float)clientHeight);

    ACHILLES_IF_DESTROYING_RETURN();

    // Execute direct command list
    uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    // Execute compute command list
    fenceValue = computeQueue->ExecuteCommandList(computeList);
    computeQueue->WaitForFenceValue(fenceValue);
}

void Helios::OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt)
{
    playerShip->OnKeyboard(kbt, kb, dt);

    if (kbt.pressed.Escape)
    {
        PostQuitMessage(0);
    }

    if (kbt.pressed.F1)
    {
        showDebug = !showDebug;
    }
}

void Helios::OnMouse(DirectX::Mouse::ButtonStateTracker mt, MouseData md, DirectX::Mouse::State state, float dt)
{
    playerShip->OnMouse(mt, md, state, dt);
}

void Helios::OnGamePad(float dt)
{
    DirectX::GamePad::State state = gamepad->GetState(0, DirectX::GamePad::DEAD_ZONE_CIRCULAR);
    if (state.IsConnected())
        playerShip->OnGamePad(state, dt);
}
