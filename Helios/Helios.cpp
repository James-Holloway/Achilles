#include "Helios.h"
#include "Achilles/shaders/BlinnPhong.h"

Helios::Helios(std::wstring _name, uint32_t width, uint32_t height) : Achilles(_name, width, height)
{
}

void Helios::OnUpdate(float deltaTime)
{
    if (playerShip != nullptr)
    {
        playerShip->OnUpdate(deltaTime);
    }
}

void Helios::OnPostRender(float deltaTime)
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
        }
    }
    ImGui::End();
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

    mainScene->AddObjectToScene(playerShip);

    // Setup light
    sun = Object::CreateLightObject(L"Sun", nullptr);
    DirectionalLight sunDirLight;
    sunDirLight.Color = Color(0.9f, 0.9f, 0.7f);
    sun->AddLight(sunDirLight);
    sun->SetLocalRotation(Quaternion::CreateFromYawPitchRoll(toRad(90.0f), toRad(45.0f), 0.0f));
    mainScene->AddObjectToScene(sun);

    ACHILLES_IF_DESTROYING_RETURN();

    // Load Skymap
    std::shared_ptr<Texture> skymapPano = std::make_shared<Texture>(TextureUsage::Linear, L"Outdoor HDRI 064 Pano");
    commandList->LoadTextureFromContent(*skymapPano, L"Skymap", TextureUsage::Linear);
    std::shared_ptr<Texture> skymapCubemap = CreateCubemap(1024, 1024);
    skymapCubemap->SetName(L"Outdoor HDRI 64 Cubemap");
    computeList->PanoToCubemap(*skymapCubemap, *skymapPano);
    // Set the skybox's cubemap texture
    skydome->GetMaterial().SetTexture(L"Cubemap", skymapCubemap);
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
