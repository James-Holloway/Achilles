#include "Thetis.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Thetis::Thetis(std::wstring name) : Achilles(name)
{

}

void Thetis::OnUpdate(float deltaTime)
{
    Vector3 rotation = cube->GetLocalRotation();
    rotation.x = fmod(rotation.x + deltaTime * 2 * cubeRotationSpeed, Achilles2Pi);
    rotation.y = fmod(rotation.y + deltaTime * 0.75f * cubeRotationSpeed, Achilles2Pi);
    cube->SetLocalRotation(rotation);

    cube->SetLocalScale(Vector3(1.0f + (0.5f * sinf((float)totalElapsedSeconds * 2.0f))));
}

void Thetis::OnRender(float deltaTime)
{

}

static int radioIndex = 0;
static bool DrawImGuiObjectTree(std::shared_ptr<Object> object)
{
    ImGui::Indent(8);
    if (ImGui::RadioButton(WStringToString(object->GetName() + L"##" + std::to_wstring(radioIndex++)).c_str(), Thetis::selectedPropertiesObject == object))
    {
        if (Thetis::selectedPropertiesObject != object)
            Thetis::selectedPropertiesObject = object;
        else
            Thetis::selectedPropertiesObject = nullptr;
    }
    return true;
}
static void DrawImGuiObjectTreeUp(std::shared_ptr<Object> object)
{
    ImGui::Unindent(8);
}

void Thetis::DrawImGuiPerformance()
{
    if (ImGui::Begin("Performance", &showPerformance, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(300, 235), ImGuiCond_Always);

        ImGui::Text("FPS: %.2f", lastFPS);

        if (ImGui::BeginTabBar("PerformanceTabBar"))
        {
            if (ImGui::BeginTabItem("Frame Time"))
            {
                if (ImPlot::BeginPlot("##PerformanceFrameTime", ImVec2(-1, 150)))
                {
                    ImPlot::SetupAxes(NULL, "Frame time (ms)", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_Invert, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 150.0, ImPlotCond_Always); // 150 points of data, locked
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 35, ImPlotCond_Always); // 35 ms max plot size, initial only

                    std::vector<double> frameTimes{};
                    frameTimes.reserve(historicalFrameTimes.size() - 1);
                    for (double ft : historicalFrameTimes)
                    {
                        frameTimes.push_back(ft * 1e3); // seconds to ms
                    }

                    ImPlot::PlotShaded<double>("", frameTimes.data(), (int)frameTimes.size());
                    ImPlot::EndPlot();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("FPS"))
            {
                if (ImPlot::BeginPlot("##PerformanceFramesPerSecond", ImVec2(-1, 150)))
                {
                    ImPlot::SetupAxes(NULL, "Frames per second", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_Invert, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 150.0, ImPlotCond_Always); // 150 points of data, locked
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1000, ImPlotCond_Always); // 1000 fps max plot size, initial only

                    std::vector<double> frames{};
                    frames.reserve(historicalFrameTimes.size() - 1);
                    for (double ft : historicalFrameTimes)
                    {
                        frames.push_back(1.0 / ft); // seconds to fps
                    }

                    ImPlot::PlotShaded<double>("", frames.data(), (int)frames.size());
                    ImPlot::EndPlot();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void Thetis::DrawImGuiScenes()
{
    if (ImGui::Begin("Scenes", &showObjectTree, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowPos(ImVec2(0, 235), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(300, (float)(clientHeight - 235)), ImGuiCond_Always);

        if (ImGui::BeginTabBar("SceneTabBar"))
        {
            ImGui::BeginChild("padding", ImVec2(0, -(ImGui::GetTextLineHeightWithSpacing() * 3)));
            {
                for (std::shared_ptr<Scene> scene : scenes)
                {
                    std::string sceneName = WStringToString(scene->GetName());
                    if (ImGui::BeginTabItem(sceneName.c_str()))
                    {
                        ImGui::Unindent(8);
                        radioIndex = 0;
                        scene->GetObjectTree()->Traverse(DrawImGuiObjectTree, DrawImGuiObjectTreeUp, 8, 0);
                        ImGui::Indent(8);
                        ImGui::EndTabItem();
                    }
                }
            }
            ImGui::EndChild();
            ImGui::EndTabBar();
        }


        const char* meshNamePreview = meshNames[selectedMeshName].c_str();
        if (ImGui::BeginCombo("##meshNameCombo", meshNamePreview, ImGuiComboFlags_HeightRegular))
        {
            for (int i = 0; i < meshNames.size(); i++)
            {
                const bool isSelected = selectedMeshName == i;
                if (ImGui::Selectable(meshNames[i].c_str(), isSelected))
                    selectedMeshName = i;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Add"))
        {
            CreateObjectInMainScene(selectedMeshName);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(selectedPropertiesObject == nullptr);
        if (ImGui::Button("As as child"))
        {
            CreateObjectAsSelectedChild(selectedMeshName);
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
}

void Thetis::DrawImGuiProperties()
{
    if (ImGui::Begin("Properties", &showProperties, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowPos(ImVec2((float)(clientWidth - 400), 0), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(400, (float)clientHeight), ImGuiCond_Always);

        std::shared_ptr<Object> object = selectedPropertiesObject;
        if (object != nullptr)
        {
            std::string strName = WStringToString(object->GetName());
            char name[64];
            strcpy_s(name, strName.c_str());
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##PropertyName", name, 64);
            object->SetName(StringToWString(std::string(name)));

            ImGui::Separator();
            // Position, rotation and scale
            {
                float pos[3] = {
                    object->GetLocalPosition().x,
                    object->GetLocalPosition().y,
                    object->GetLocalPosition().z
                };
                if (ImGui::DragFloat3("Position: ", pos, 0.125f, 0.0, 0.0, "% .3f"))
                {
                    object->SetLocalPosition(Vector3(pos[0], pos[1], pos[2]));
                }

                Vector3 degRot = EulerToDegrees(object->GetLocalRotation());
                float rot[3] = {
                    degRot.x,
                    degRot.y,
                    degRot.z
                };
                if (ImGui::DragFloat3("Rotation: ", rot, 5.0f, -360, 360, "% .3f"))
                {
                    object->SetLocalRotation(EulerToRadians(Vector3(rot[0], rot[1], rot[2])));
                }
                float scale[3] = {
                    object->GetLocalScale().x,
                    object->GetLocalScale().y,
                    object->GetLocalScale().z
                };
                if (ImGui::DragFloat3("Scale: ", scale, 0.125f, 0.0f, 0.0f, "% .3f"))
                {
                    object->SetLocalScale(Vector3(scale[0], scale[1], scale[2]));
                }
            }

            // Buttons
            ImGui::Separator();
            if (ImGui::Button("Delete"))
            {
                DeleteSelectedObject();
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy"))
            {
                CopySelectedObject();
            }

            ImGui::Separator();
            // Extra Info
            if (ImGui::CollapsingHeader("Extra Info", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Tags
                ImGui::Text("ObjectTag: %s", WStringToString(ObjectTagToWString(object->GetTags())).c_str());

                // Knits
                uint32_t knitCount = object->GetKnitCount();
                ImGui::Text("Knit Count: %i", knitCount);
                for (uint32_t i = 0; i < knitCount; i++)
                {
                    std::string header = ("Knit #" + std::to_string(i) + "##Knit" + std::to_string(i));
                    Knit& knit = object->GetKnit(i);
                    bool hasBeenCopied = knit.mesh != nullptr && knit.mesh->HasBeenCopied();

                    if (!hasBeenCopied) // Make header red and inside text if not copied
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));

                    if (ImGui::TreeNodeEx(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (hasBeenCopied)
                            ImGui::Text("Mesh copied");
                        else
                            ImGui::Text("Mesh not copied");

                        if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::Text(("Name: " + WStringToString(knit.material.name)).c_str());
                            // TODO dynamic material properties, including textures
                            if (knit.material.HasFloat(L"Diffuse"))
                            {
                                float value = knit.material.GetFloat(L"Diffuse");
                                if (ImGui::DragFloat("Diffuse", &value, 0.025f, 0.0f, 1.0f))
                                {
                                    knit.material.SetFloat(L"Diffuse", value);
                                }
                            }
                            if (knit.material.HasFloat(L"Specular"))
                            {
                                float value = knit.material.GetFloat(L"Specular");
                                if (ImGui::DragFloat("Specular", &value, 0.025f, 0.0f, 1.0f))
                                {
                                    knit.material.SetFloat(L"Specular", value);
                                }
                            }
                            if (knit.material.HasFloat(L"SpecularPower"))
                            {
                                float value = knit.material.GetFloat(L"SpecularPower");
                                if (ImGui::DragFloat("SpecularPower", &value, 1.0f, 0.0f, 100.0f, "%.1f"))
                                {
                                    knit.material.SetFloat(L"SpecularPower", value);
                                }
                            }
                            if (knit.material.HasFloat(L"ShadingType"))
                            {
                                int value = (int)knit.material.GetFloat(L"ShadingType");
                                if (ImGui::DragInt("ShadingType", &value, 0.025f, 0, 2, "%i"))
                                {
                                    knit.material.SetFloat(L"ShadingType", (float)value);
                                }
                            }

                            ImGui::TreePop();
                        }
                        ImGui::TreePop();
                    }
                    if (!hasBeenCopied)
                        ImGui::PopStyleColor();
                }

                // Light properties
                if (object->HasTag(ObjectTag::Light))
                {
                    std::shared_ptr<LightObject> lightObject = std::dynamic_pointer_cast<LightObject>(object);

                    if (ImGui::TreeNodeEx("Global Ambient Light", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        AmbientLight& ambientLight = lightData.AmbientLight;
                        float color[3] =
                        {
                            ambientLight.Color.x,
                            ambientLight.Color.y,
                            ambientLight.Color.z,
                        };
                        if (ImGui::ColorEdit3("Color", color))
                        {
                            ambientLight.Color.x = color[0];
                            ambientLight.Color.y = color[1];
                            ambientLight.Color.z = color[2];
                        }
                        ImGui::DragFloat("Strength", &ambientLight.Strength, 0.025f, 0.0f, 1.0f);
                        ImGui::TreePop();
                    }

                    if (lightObject->HasLightType(LightType::Point))
                    {
                        if (ImGui::TreeNodeEx("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            PointLight& pointLight = lightObject->GetPointLight();
                            float color[3] =
                            {
                                pointLight.Color.x,
                                pointLight.Color.y,
                                pointLight.Color.z,
                            };
                            if (ImGui::ColorEdit3("Color", color))
                            {
                                pointLight.Color.x = color[0];
                                pointLight.Color.y = color[1];
                                pointLight.Color.z = color[2];
                            }

                            ImGui::DragFloat("Strength", &pointLight.Strength, 0.025f, 0.0f, 100.0f, "%.1f");
                            ImGui::DragFloat("Max Distance", &pointLight.MaxDistance, 1.0f, 0.0f, 10000.0f, "%.2f");
                            ImGui::DragFloat("Constant Attenuation", &pointLight.ConstantAttenuation, 0.10f, 0.0f, 2.0f, "%.2f");
                            ImGui::DragFloat("Linear Attenuation", &pointLight.LinearAttenuation, 0.0025f, 0.0f, 2.0f, "%.4f");
                            ImGui::DragFloat("Quadratic Attenuation", &pointLight.QuadraticAttenuation, 0.00025f, 0.0f, 2.0f, "%.7f");
                            ImGui::TreePop();
                        }
                    }
                    if (lightObject->HasLightType(LightType::Spot))
                    {
                        if (ImGui::TreeNodeEx("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            SpotLight& spotLight = lightObject->GetSpotLight();
                            float color[3] =
                            {
                                spotLight.Light.Color.x,
                                spotLight.Light.Color.y,
                                spotLight.Light.Color.z,
                            };
                            if (ImGui::ColorEdit3("Color", color))
                            {
                                spotLight.Light.Color.x = color[0];
                                spotLight.Light.Color.y = color[1];
                                spotLight.Light.Color.z = color[2];
                            }

                            ImGui::DragFloat("Strength", &spotLight.Light.Strength, 0.025f, 0.0f, 100.0f, "%.3f");
                            ImGui::DragFloat("Max Distance", &spotLight.Light.MaxDistance, 1.0f, 0.0f, 10000.0f, "%.2f");
                            ImGui::DragFloat("Constant Attenuation", &spotLight.Light.ConstantAttenuation, 0.10f, 0.0f, 2.0f, "%.2f");
                            ImGui::DragFloat("Linear Attenuation", &spotLight.Light.LinearAttenuation, 0.025f, 0.0f, 2.0f, "%.4f");
                            ImGui::DragFloat("Quadratic Attenuation", &spotLight.Light.QuadraticAttenuation, 0.0025f, 0.0f, 2.0f, "%.7f");
                            float innerAngle = toDeg(spotLight.InnerSpotAngle);
                            if (ImGui::DragFloat("Inner Angle", &innerAngle, 0.25f, 0.0f, 89.9f, "%.2f"))
                            {
                                spotLight.InnerSpotAngle = toRad(innerAngle);
                            }
                            float outerAngle = toDeg(spotLight.OuterSpotAngle);
                            if (ImGui::DragFloat("Outer Angle", &outerAngle, 0.25F, 0.0f, 89.9f, "%.2f"))
                            {
                                spotLight.OuterSpotAngle = toRad(outerAngle);
                            }
                            ImGui::TreePop();
                        }
                    }
                }
            }

        }
    }
    ImGui::End();
}

void Thetis::OnPostRender(float deltaTime)
{
    if (showPerformance)
    {
        DrawImGuiPerformance();
    }

    if (showObjectTree)
    {
        DrawImGuiScenes();
    }
    if (showProperties)
    {
        DrawImGuiProperties();
    }
}

void Thetis::OnResize(int newWidth, int newHeight)
{
    camera->UpdateViewport(newWidth, newHeight);
    camera->dirtyProjMatrix = true;
}

void Thetis::LoadContent()
{
    // Get command queue + list
    std::shared_ptr<CommandQueue> commandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

    // Create camera
    camera = std::make_shared<Camera>(L"camera", clientWidth, clientHeight);
    Camera::mainCamera = camera;
    camera->fov = 60;
    camera->position = Vector3(5, 0, 0);
    camera->rotation = EulerToRadians(Vector3(0, -90, 0));

    // Create shaders
    std::shared_ptr<Shader> posColShader = GetPosColShader(device);
    std::shared_ptr<Shader> posTexturedShader = PosTextured::GetPosTexturedShader(device);

    // Create cube
    auto cubeMesh = std::make_shared<Mesh>(L"cube", commandList, (void*)posColCubeVertices, (UINT)_countof(posColCubeVertices), sizeof(PosColVertex), posColCubeIndices, (UINT)_countof(posColCubeIndices), posColShader);
    cube = Object::CreateObject(L"cube");
    cube->SetMesh(0, cubeMesh);
    cube->SetLocalPosition(Vector3(-1, 0, 0));
    // mainScene->AddObjectToScene(cube);
    cube->SetActive(false);

    miniCube = Object::CreateObject(L"mini cube");
    miniCube->SetMesh(0, cubeMesh);
    cube->AddChild(miniCube);
    miniCube->SetLocalPosition(Vector3(0, 2, 0));
    miniCube->SetLocalScale(Vector3(0.25f, 0.25f, 0.25f));

    // Create floor quad
    auto floorMesh = std::make_shared<Mesh>(L"floor quad", commandList, (void*)PosTextured::posTexturedQuadVertices, (UINT)_countof(PosTextured::posTexturedQuadVertices), sizeof(PosTextured::PosTexturedVertex), PosTextured::posTexturedQuadIndices, (UINT)_countof(PosTextured::posTexturedQuadIndices), posTexturedShader);

    floorQuad = Object::CreateObject(L"floor quad");
    floorQuad->SetMesh(0, floorMesh);
    floorQuad->SetLocalPosition(Vector3(0, -2, 0));
    floorQuad->SetLocalRotation(EulerToRadians(Vector3(-90, 0, 0)));
    floorQuad->SetLocalScale(Vector3(3, 3, 3));

    std::shared_ptr<Texture> floorTexture = std::make_shared<Texture>();
    std::wstring floorTexturePath = GetContentDirectoryW() + L"textures/MyUVSquare.png";
    commandList->LoadTextureFromFile(*floorTexture, floorTexturePath, TextureUsage::Albedo);
    floorQuad->GetMaterial().textures.insert({ L"MainTexture", floorTexture });

    mainScene->AddObjectToScene(floorQuad);

    // Load content/models files into meshNames
    PopulateMeshNames();

    // Execute command list
    uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);
}

void Thetis::UnloadContent()
{

}

void Thetis::OnKeyboard(Keyboard::KeyboardStateTracker kbt, Keyboard::State kb, float dt)
{
    if (kbt.pressed.F1)
    {
        if (showPerformance && showObjectTree && showProperties)
            showPerformance = showObjectTree = showProperties = false;
        else
            showPerformance = showObjectTree = showProperties = true;
    }
    if (kbt.pressed.F2)
    {
        showPerformance = !showPerformance;
    }
    if (kbt.pressed.F3)
    {
        showObjectTree = !showObjectTree;
    }
    if (kbt.pressed.F4)
    {
        showProperties = !showProperties;
    }

    if (kbt.pressed.PageUp)
    {
        camera->fov -= 5;
        OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
        camera->dirtyProjMatrix = true;
    }
    if (kbt.pressed.PageDown)
    {
        camera->fov += 5;
        OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
        camera->dirtyProjMatrix = true;
    }

    float cameraRotationAmount = 15;
    if (kbt.pressed.Up)
    {
        camera->RotateEuler(Vector3(-toRad(cameraRotationAmount), 0, 0));
    }
    if (kbt.pressed.Down)
    {
        camera->RotateEuler(Vector3(toRad(cameraRotationAmount), 0, 0));
    }
    if (kbt.pressed.Right)
    {
        camera->RotateEuler(Vector3(0, toRad(cameraRotationAmount), 0));
    }
    if (kbt.pressed.Left)
    {
        camera->RotateEuler(Vector3(0, -toRad(cameraRotationAmount), 0));
    }

    float movementSpeed = cameraBaseMoveSpeed;
    if (kb.LeftControl)
        movementSpeed *= 0.5f;
    if (kb.LeftShift)
        movementSpeed *= 2.0f;

    movementSpeed *= dt;

    if (kb.W)
    {
        camera->MoveRelative(Vector3(0, 0, movementSpeed));
    }
    if (kb.S)
    {
        camera->MoveRelative(Vector3(0, 0, -movementSpeed));
    }
    if (kb.D)
    {
        camera->MoveRelative(Vector3(movementSpeed, 0, 0));
    }
    if (kb.A)
    {
        camera->MoveRelative(Vector3(-movementSpeed, 0, 0));
    }
    if (kb.E)
    {
        camera->MoveRelative(Vector3(0, movementSpeed, 0));
    }
    if (kb.Q)
    {
        camera->MoveRelative(Vector3(0, -movementSpeed, 0));
    }
}

void Thetis::OnMouse(Mouse::ButtonStateTracker mt, MouseData md, Mouse::State state, float dt)
{
    if (mt.rightButton)
    {
        float cameraRotationSpeed = toRad(45) * dt * mouseSensitivity;
        camera->RotateEuler(Vector3(cameraRotationSpeed * md.mouseYDelta, cameraRotationSpeed * md.mouseXDelta, 0));
    }
}


void Thetis::PopulateMeshNames()
{
    meshNames.clear();
    meshNamesWide.clear();
    for (const auto& file : std::filesystem::directory_iterator(GetContentDirectoryW() + L"models/"))
    {
        if (file.is_directory())
            continue;
        auto path = file.path();
        meshNames.push_back(path.filename().string());
        meshNamesWide.push_back(path.filename().wstring());
    }
}

void Thetis::CreateObjectInMainScene(uint32_t meshNameIndex)
{
    std::shared_ptr<Object> object = Object::CreateObjectsFromContentFile(meshNamesWide[meshNameIndex], SimpleDiffuse::GetSimpleDiffuseShader(device));
    mainScene->AddObjectToScene(object);
}

void Thetis::CreateObjectAsSelectedChild(uint32_t meshNameIndex)
{
    if (selectedPropertiesObject == nullptr)
    {
        CreateObjectInMainScene(meshNameIndex);
        return;
    }
    std::shared_ptr<Object> object = Object::CreateObjectsFromContentFile(meshNamesWide[meshNameIndex], SimpleDiffuse::GetSimpleDiffuseShader(device));
    selectedPropertiesObject->AddChild(object);
}

void Thetis::DeleteSelectedObject()
{
    if (selectedPropertiesObject == nullptr)
        return;

    selectedPropertiesObject->SetParent(nullptr);

    selectedPropertiesObject = nullptr;
}

std::shared_ptr<Object> Thetis::CopySelectedObject()
{
    if (selectedPropertiesObject == nullptr)
        return nullptr;

    return selectedPropertiesObject->Clone();
}