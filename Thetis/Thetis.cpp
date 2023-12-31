#include "Thetis.h"
#include "Achilles/AchillesShaders.h"
#include "shaders/Water.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

#define THETIS_TONEMAPPER_COMBO(Name) \
if (ImGui::Selectable(PPToneMapping::ToneMappers::GetToneMapperName(PPToneMapping::ToneMappers:: Name).c_str(), postProcessing->ToneMapper == PPToneMapping::ToneMappers:: Name)) \
{ \
    postProcessing->ToneMapper = PPToneMapping::ToneMappers:: Name; \
} \
if (postProcessing->ToneMapper == PPToneMapping::ToneMappers:: Name) \
{ \
    ImGui::SetItemDefaultFocus(); \
}

#define THETIS_SHADOWMAP_COMBO(Size) \
if (ImGui::Selectable(std::to_string(Size).c_str(), shadowMapSize == Size)) \
{ \
    postPresentFunctions.push_back([=]() { shadowCamera->ResizeShadowMaps(Size, Size); }); \
} \
if (shadowMapSize == Size) \
{ \
    ImGui::SetItemDefaultFocus(); \
}

Thetis::Thetis(std::wstring _name, uint32_t width, uint32_t height) : Achilles(_name, width, height)
{

}

Thetis::~Thetis()
{

}

void Thetis::OnUpdate(float deltaTime)
{

}

void Thetis::OnRender(float deltaTime)
{
    ScopedTimer _prof(L"OnRender");
    if (drawDebugBoundingBox && selectedPropertiesObject != nullptr)
    {
        BoundingOrientedBox obb = selectedPropertiesObject->GetWorldBoundingBox();
        BoundingBox aabb = selectedPropertiesObject->GetWorldAABB();
        debugBoundingBox->SetLocalPosition(obb.Center);
        debugBoundingBox->SetLocalRotation(obb.Orientation);
        debugBoundingBox->SetLocalScale(obb.Extents);
        QueueObjectDraw(debugBoundingBox, true);

        debugBoundingBoxCenter->SetLocalPosition(obb.Center);
        debugBoundingBoxCenter->SetLocalRotation(Quaternion::Identity);
        debugBoundingBoxCenter->SetLocalScale(Vector3(0.05f));
        QueueObjectDraw(debugBoundingBoxCenter, true);

        debugBoundingBoxAABB->SetLocalPosition(aabb.Center);
        debugBoundingBoxAABB->SetLocalRotation(Quaternion::Identity);
        debugBoundingBoxAABB->SetLocalScale(aabb.Extents);
        QueueObjectDraw(debugBoundingBoxAABB, true);
    }
}

static int radioIndex = 0;
static bool DrawImGuiObjectTree(std::shared_ptr<Object> object)
{
    ImGui::Indent(8);

    bool isActive = object->IsActive();
    if (!isActive)
        ImGui::PushStyleColor(ImGuiCol_Text, AchillesImGui::Color_TextDisabled);

    std::string id = WStringToString(object->GetName() + L"##" + std::to_wstring(radioIndex++));
    if (ImGui::RadioButton(id.c_str(), Thetis::selectedPropertiesObject == object))
    {
        if (Thetis::selectedPropertiesObject != object)
            Thetis::selectedPropertiesObject = object;
        else
            Thetis::selectedPropertiesObject = nullptr;
    }

    if (!isActive)
        ImGui::PopStyleColor();

    return true;
}
static void DrawImGuiObjectTreeUp(std::shared_ptr<Object> object)
{
    ImGui::Unindent(8);
}

void Thetis::DrawImGuiTextureProperty(std::shared_ptr<Object> object, uint32_t knitIndex, std::wstring name)
{
    std::string id = WStringToString(name);
    ImGui::PushID(id.c_str());

    ImGui::Text(id.c_str());
    std::shared_ptr<Texture> texture = object->GetMaterial(knitIndex).GetTexture(name);

    ImVec2 size(192, 192);

    if (texture == nullptr)
    {
        if (ImGui::Button("No Texture", size))
        {
            SelectTexture([=](std::shared_ptr<Texture> newTexture) { object->GetMaterial(knitIndex).SetTexture(name, newTexture); });
        }
    }
    else
    {
        float width = 0.0f;
        float height = 0.0f;
        texture->GetSize(width, height);

        if (width > height)
            size.y = (height / width) * size.y;
        else if (height > width)
            size.x = (width / height) * size.x;

        if (AchillesImGui::ImageButton(texture, size, ImVec2(0, 0), ImVec2(1, 1), 2))
        {
            SelectTexture([=](std::shared_ptr<Texture> newTexture) { object->GetMaterial(knitIndex).SetTexture(name, newTexture); });
        }
    }
    ImGui::PopID();
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

                    if (historicalFrameTimes.size() > 0)
                    {
                        std::vector<double> frameTimes{};
                        frameTimes.reserve(historicalFrameTimes.size() - 1);
                        for (double ft : historicalFrameTimes)
                        {
                            frameTimes.push_back(ft * 1e3); // seconds to ms
                        }

                        ImPlot::PlotShaded<double>("", frameTimes.data(), (int)frameTimes.size());
                    }
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

                    if (historicalFrameTimes.size() > 0)
                    {
                        std::vector<double> frames{};
                        frames.reserve(historicalFrameTimes.size() - 1);
                        for (double ft : historicalFrameTimes)
                        {
                            frames.push_back(1.0 / ft); // seconds to fps
                        }

                        ImPlot::PlotShaded<double>("", frames.data(), (int)frames.size());
                    }
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
    static bool shouldSetSceneTab = false;
    if (ImGui::Begin("Scenes", &showObjectTree, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowPos(ImVec2(0, 235), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(300, (float)(clientHeight - 235)), ImGuiCond_Always);

        unsigned int bottomPaddingLines = 2;
        ImGui::BeginChild("padding", ImVec2(0, -(ImGui::GetTextLineHeightWithSpacing() * (2 + bottomPaddingLines))));
        {
            if (selectedPropertiesScene == nullptr)
                selectedPropertiesScene = mainScene;

            // Selected Scene Tabs
            std::string selectedSceneName = "[None]";
            if (selectedPropertiesScene != nullptr)
                selectedSceneName = WStringToString(selectedPropertiesScene->GetName());

            // Prevent scene switch this frame (i.e. when input name changed)
            bool preventSceneSwitch = false;

            // Scene Properties + Operations
            if (selectedPropertiesScene != nullptr)
            {
                std::shared_ptr<Scene> scene = selectedPropertiesScene;
                // Scene Active + Name
                {
                    // Scene Active
                    bool sceneActive = scene->IsActive();
                    if (ImGui::Checkbox("##SceneActive", &sceneActive))
                        scene->SetActive(sceneActive);

                    // Input Name Text
                    std::string strName = WStringToString(scene->GetName());
                    char name[64];
                    strcpy_s(name, strName.c_str());
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(-1);

                    if (ImGui::InputText("##SceneName", name, 64))
                    {
                        scene->SetName(StringToWString(std::string(name)));
                        shouldSetSceneTab = true;
                        preventSceneSwitch = true;
                    }
                }

                // Scene Operation Buttons
                {
                    if (ImGui::Button("New"))
                    {
                        std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
                        newScene->SetActive(true);
                        AddScene(newScene);
                        selectedPropertiesScene = newScene;
                    }
                    ImGui::SameLine();

                    // Disable Delete when it is the Main Scene
                    ImGui::BeginDisabled(scene == mainScene);
                    if (ImGui::Button("Delete"))
                    {
                        RemoveScene(scene);
                        scene = nullptr;
                    }
                    ImGui::EndDisabled();
                }
            }

            if (ImGui::BeginTabBar("SceneTabBar", ImGuiTabBarFlags_FittingPolicyScroll))
            {
                uint32_t i = 0;
                std::deque<std::shared_ptr<Scene>> orderedScenes;
                for (std::shared_ptr<Scene> scene : scenes)
                {
                    if (scene == mainScene)
                        continue;
                    if (scene == nullptr)
                        continue;
                    orderedScenes.push_back(scene);
                }

                std::sort(orderedScenes.begin(), orderedScenes.end(), SceneSharedPtrNameSort);

                if (mainScene != nullptr) // shouldn't be nullptr but we'll do the check anyway
                    orderedScenes.push_front(mainScene);

                for (std::shared_ptr<Scene> scene : orderedScenes)
                {
                    std::string sceneName = WStringToString(scene->GetName());
                    ImGuiTabItemFlags flags = ImGuiTabItemFlags_NoPushId;
                    if (shouldSetSceneTab && scene == selectedPropertiesScene)
                    {
                        flags |= ImGuiTabItemFlags_SetSelected;
                        shouldSetSceneTab = false;
                    }

                    bool isActive = scene->IsActive();
                    if (!isActive)
                        ImGui::PushStyleColor(ImGuiCol_Text, AchillesImGui::Color_TextDisabled);

                    if (ImGui::BeginTabItem((sceneName + "##" + std::to_string(i++)).c_str(), (bool*)0, flags))
                    {
                        if (!isActive)
                            ImGui::PopStyleColor();

                        // If the scene has just changed
                        if (selectedPropertiesScene != scene)
                        {
                            selectedPropertiesObject = nullptr; // deselect properties object
                        }
                        if (!preventSceneSwitch)
                            selectedPropertiesScene = scene;

                        // Draw Object Tree
                        ImGui::Unindent(8);
                        radioIndex = 0;
                        selectedPropertiesScene->GetObjectTree()->Traverse(DrawImGuiObjectTree, DrawImGuiObjectTreeUp, 8, 0);
                        ImGui::Indent(8);

                        ImGui::EndTabItem();
                    }
                    else
                    {
                        if (!isActive)
                            ImGui::PopStyleColor();
                    }
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();

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
            CreateObjectInSelectedScene(selectedMeshName);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(selectedPropertiesObject == nullptr);
        if (ImGui::Button("As as child"))
        {
            CreateObjectAsSelectedChild(selectedMeshName);
        }
        ImGui::EndDisabled();
        if (ImGui::Button("Add Water Plane"))
        {
            AddWaterPlane();
        }
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
            bool isActive = object->IsActive();
            if (ImGui::Checkbox("##Active", &isActive))
                object->SetActive(isActive);

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("IsActive");

            std::string strName = WStringToString(object->GetName());
            char name[64];
            strcpy_s(name, strName.c_str());
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##PropertyName", name, 64))
            {
                object->SetName(StringToWString(std::string(name)));
            }

            ImGui::Separator();
            // Position, rotation and scale
            if (ImGui::BeginTabBar("localOrWorld", ImGuiTabBarFlags_FittingPolicyScroll))
            {
                if (ImGui::BeginTabItem("Local"))
                {
                    float pos[3] = {
                        object->GetLocalPosition().x,
                        object->GetLocalPosition().y,
                        object->GetLocalPosition().z
                    };
                    if (ImGui::DragFloat3("Position", pos, 0.125f, 0.0, 0.0, "% .3f"))
                    {
                        object->SetLocalPosition(Vector3(pos[0], pos[1], pos[2]));
                    }

                    Quaternion quatRot = object->GetLocalRotation();
                    float quat[4]
                    {
                        quatRot.x,
                        quatRot.y,
                        quatRot.z,
                        quatRot.w,
                    };
                    if (ImGui::DragFloat4("Quaternion", quat, 0.05f, -1.0f, 1.0f, "%.3f"))
                    {
                        object->SetLocalRotation(Quaternion(quat[0], quat[1], quat[2], quat[3]));
                    }

                    Vector3 degRot = EulerToDegrees(object->GetLocalRotation().ToEuler());
                    float rot[3] = {
                        degRot.x,
                        degRot.y,
                        degRot.z
                    };
                    if (ImGui::DragFloat3("Euler Angles", rot, 5.0f, -360, 360, "% .3f"))
                    {
                        object->SetLocalRotation(Quaternion::CreateFromYawPitchRoll(EulerToRadians(Vector3(rot[0], rot[1], rot[2]))));
                    }

                    Vector3 degRotE = EulerToDegrees(object->GetLocalEulerRotation());
                    float rotE[3] = {
                        degRotE.x,
                        degRotE.y,
                        degRotE.z
                    };
                    if (ImGui::DragFloat3("Local Euler Rotation", rotE, 5.0f, -360, 360, "% .3f"))
                    {
                        object->SetLocalEulerRotation(EulerToRadians(Vector3(rotE[0], rotE[1], rotE[2])));
                    }

                    float scale[3] = {
                        object->GetLocalScale().x,
                        object->GetLocalScale().y,
                        object->GetLocalScale().z
                    };
                    if (ImGui::DragFloat3("Scale", scale, 0.125f, 0.0f, 0.0f, "% .3f"))
                    {
                        object->SetLocalScale(Vector3(scale[0], scale[1], scale[2]));
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World"))
                {
                    float pos[3] = {
                        object->GetWorldPosition().x,
                        object->GetWorldPosition().y,
                        object->GetWorldPosition().z
                    };
                    if (ImGui::DragFloat3("Position", pos, 0.125f, 0.0, 0.0, "% .3f"))
                    {
                        object->SetWorldPosition(Vector3(pos[0], pos[1], pos[2]));
                    }

                    Quaternion quatRot = object->GetWorldRotation();
                    float quat[4]
                    {
                        quatRot.x,
                        quatRot.y,
                        quatRot.z,
                        quatRot.w,
                    };
                    if (ImGui::DragFloat4("Quaternion", quat, 0.05f, -1.0f, 1.0f, "%.3f"))
                    {
                        object->SetWorldRotation(Quaternion(quat[0], quat[1], quat[2], quat[3]));
                    }

                    Vector3 degRot = EulerToDegrees(object->GetWorldRotation().ToEuler());
                    float rot[4] = {
                        degRot.x,
                        degRot.y,
                        degRot.z
                    };
                    if (ImGui::DragFloat3("Euler Angles", rot, 5.0f, -360, 360, "% .3f"))
                    {
                        object->SetWorldRotation(Quaternion::CreateFromYawPitchRoll(EulerToRadians(Vector3(rot[0], rot[1], rot[2]))));
                    }

                    Vector3 degRotE = EulerToDegrees(object->GetLocalEulerRotation());
                    float rotE[3] = {
                        degRotE.x,
                        degRotE.y,
                        degRotE.z
                    };
                    if (ImGui::DragFloat3("Local Euler Rotation", rotE, 5.0f, -360, 360, "% .3f"))
                    {
                        object->SetLocalEulerRotation(EulerToRadians(Vector3(rotE[0], rotE[1], rotE[2])));
                    }

                    float scale[3] = {
                        object->GetWorldScale().x,
                        object->GetWorldScale().y,
                        object->GetWorldScale().z
                    };
                    if (ImGui::DragFloat3("Scale", scale, 0.125f, 0.0f, 0.0f, "% .3f"))
                    {
                        object->SetWorldScale(Vector3(scale[0], scale[1], scale[2]));
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Local Matrix"))
                {
                    Matrix matrix = object->GetLocalMatrix();
                    bool matrixChanged = ImGui::InputFloat4("##Matrix1", matrix.m[0], "%.3f");
                    matrixChanged |= ImGui::InputFloat4("##Matrix2", matrix.m[1], "%.3f");
                    matrixChanged |= ImGui::InputFloat4("##Matrix3", matrix.m[2], "%.3f");
                    matrixChanged |= ImGui::InputFloat4("##Matrix4", matrix.m[3], "%.3f");
                    if (matrixChanged)
                    {
                        object->SetLocalMatrix(matrix);
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World Matrix"))
                {
                    Matrix matrix = object->GetWorldMatrix();
                    bool matrixChanged = ImGui::InputFloat4("##Matrix1", matrix.m[0], "%.3f");
                    matrixChanged |= ImGui::InputFloat4("##Matrix2", matrix.m[1], "%.3f");
                    matrixChanged |= ImGui::InputFloat4("##Matrix3", matrix.m[2], "%.3f");
                    matrixChanged |= ImGui::InputFloat4("##Matrix4", matrix.m[3], "%.3f");
                    if (matrixChanged)
                    {
                        object->SetWorldMatrix(matrix);
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Local OBB"))
                {
                    BoundingOrientedBox obb = object->GetBoundingBox();
                    bool obbChanged = ImGui::InputFloat3("Center", &obb.Center.x);
                    obbChanged |= ImGui::InputFloat3("Extents", &obb.Extents.x);
                    obbChanged |= ImGui::InputFloat4("Orientation", &obb.Orientation.x);

                    if (obbChanged)
                    {
                        object->SetBoundingBox(obb);
                    }

                    ImGui::Checkbox("Frustum Culling", &frustumCulling);
                    ImGui::Checkbox("Draw Debug Bounding Box", &drawDebugBoundingBox);

                    std::shared_ptr<Camera> camera = Camera::debugShadowCamera == nullptr ? Camera::mainCamera : Camera::debugShadowCamera;
                    if (camera != nullptr)
                    {
                        ImGui::Text(object->ShouldDraw(camera->GetFrustum()) ? "Drawing" : (frustumCulling ? "Culled" : "Would be culled"));
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World OBB"))
                {
                    BoundingOrientedBox obb = object->GetWorldBoundingBox();
                    ImGui::BeginDisabled(true);
                    ImGui::InputFloat3("Center", &obb.Center.x);
                    ImGui::InputFloat3("Extents", &obb.Extents.x);
                    ImGui::InputFloat4("Orientation", &obb.Orientation.x);
                    ImGui::EndDisabled();

                    ImGui::Checkbox("Frustum Culling", &frustumCulling);
                    ImGui::Checkbox("Draw Debug Bounding Box", &drawDebugBoundingBox);

                    std::shared_ptr<Camera> camera = Camera::debugShadowCamera == nullptr ? Camera::mainCamera : Camera::debugShadowCamera;
                    if (camera != nullptr)
                    {
                        ImGui::Text(object->ShouldDraw(camera->GetFrustum()) ? "Drawing" : "Culled");
                    }

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            // Buttons
            ImGui::Separator();
            if (ImGui::Button("Delete"))
            {
                DeleteSelectedObject();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clone"))
            {
                CopySelectedObject();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Parent"))
            {
                ClearSelectedParent();
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
                            if (knit.material.HasVector(L"UVScaleOffset"))
                            {
                                Vector4 scaleOffset = knit.material.GetVector(L"UVScaleOffset");
                                bool hasChanged = ImGui::DragFloat2("UV Scale", &scaleOffset.x, 0.1f, -100.0f, 100.0f, "%.3f");
                                hasChanged |= ImGui::DragFloat2("UV Offset", &scaleOffset.z, 0.01f, -100.0f, 100.0f, "%.3f");
                                if (hasChanged)
                                {
                                    knit.material.SetVector(L"UVScaleOffset", scaleOffset);
                                }
                            }

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
                            if (knit.material.HasFloat(L"EmissionStrength"))
                            {
                                float value = knit.material.GetFloat(L"EmissionStrength");
                                if (ImGui::DragFloat("EmissionStrength", &value, 0.025f, 0.0f, 1.0f, "%.2f"))
                                {
                                    knit.material.SetFloat(L"EmissionStrength", value);
                                }
                            }
                            if (knit.material.HasFloat(L"ShadingType"))
                            {
                                int value = (int)knit.material.GetFloat(L"ShadingType");
                                if (ImGui::DragInt("ShadingType", &value, 0.025f, 0, 3, "%i"))
                                {
                                    knit.material.SetFloat(L"ShadingType", (float)value);
                                }
                            }

                            if (knit.material.HasVector(L"Color"))
                            {
                                Color color = knit.material.GetVector(L"Color");
                                float col[]
                                {
                                    color.x,
                                    color.y,
                                    color.z,
                                    color.w
                                };
                                if (ImGui::ColorEdit4("Color", col, ImGuiColorEditFlags_AlphaBar))
                                {
                                    knit.material.SetVector(L"Color", Vector4(col[0], col[1], col[2], col[3]));
                                }
                            }

                            // Textures
                            if (ImGui::CollapsingHeader("Textures"))
                            {
                                ImGui::Indent(4.0f);

                                // Diffuse
                                DrawImGuiTextureProperty(object, i, L"MainTexture");

                                // Normal
                                DrawImGuiTextureProperty(object, i, L"NormalTexture");

                                // Emission
                                DrawImGuiTextureProperty(object, i, L"EmissionTexture");

                                ImGui::Unindent(4.0f);
                            }

                            ImGui::TreePop();
                        }
                        ImGui::TreePop();
                    }
                    if (!hasBeenCopied)
                        ImGui::PopStyleColor();
                }

                ImGui::Separator();

                if (object->HasTag(ObjectTag::Mesh))
                {
                    bool castsShadows = object->CastsShadows();
                    if (ImGui::Checkbox("Casts Shadows", &castsShadows))
                    {
                        object->SetCastsShadows(castsShadows);
                    }

                    bool receivesShadows = object->ReceivesShadows();
                    if (ImGui::Checkbox("Receives Shadows", &receivesShadows))
                    {
                        object->SetReceiveShadows(receivesShadows);
                    }
                }

                // Light properties
                if (object->HasTag(ObjectTag::Light))
                {
                    std::shared_ptr<LightObject> lightObject = std::dynamic_pointer_cast<LightObject>(object);

                    bool shadowCaster = lightObject->IsShadowCaster();
                    if (ImGui::Checkbox("Is Shadow Caster", &shadowCaster))
                    {
                        lightObject->SetIsShadowCaster(shadowCaster);
                    }

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
                                pointLight.Light.Color.x,
                                pointLight.Light.Color.y,
                                pointLight.Light.Color.z,
                            };
                            if (ImGui::ColorEdit3("Color", color))
                            {
                                pointLight.Light.Color.x = color[0];
                                pointLight.Light.Color.y = color[1];
                                pointLight.Light.Color.z = color[2];
                            }

                            ImGui::DragFloat("Strength", &pointLight.Light.Strength, 0.025f, 0.0f, 100.0f, "%.1f");
                            ImGui::DragFloat("Max Distance", &pointLight.Light.MaxDistance, 1.0f, 0.0f, 10000.0f, "%.2f");
                            ImGui::Separator();
                            ImGui::DragFloat("Constant Attenuation", &pointLight.Light.ConstantAttenuation, 0.10f, 0.0f, 2.0f, "%.2f");
                            ImGui::DragFloat("Linear Attenuation", &pointLight.Light.LinearAttenuation, 0.0025f, 0.0f, 2.0f, "%.4f");
                            ImGui::DragFloat("Quadratic Attenuation", &pointLight.Light.QuadraticAttenuation, 0.00025f, 0.0f, 2.0f, "%.7f");
                            ImGui::Separator();
                            ImGui::DragFloat("Rank", &pointLight.Light.Rank, 0.25f, -25.0f, 25.0f, "%.0f");

                            ImGui::Separator();
                            std::shared_ptr<ShadowCamera> shadowCamera = lightObject->GetShadowCamera(LightType::Point, nullptr);
                            if (shadowCamera && shadowCamera->GetShadowMap() != nullptr)
                            {
                                static uint32_t selectedPointCubeDirection = 0;

                                std::string selectedPointCubeDirectionStr = std::to_string(selectedPointCubeDirection);
                                if (ImGui::BeginCombo("View Direction", selectedPointCubeDirectionStr.c_str()))
                                {
                                    for (uint32_t c = 0; c < 6; c++)
                                    {
                                        if (ImGui::Selectable(std::to_string(c).c_str(), selectedPointCubeDirection == c))
                                        {
                                            selectedPointCubeDirection = c;
                                        }
                                        if (selectedPointCubeDirection == c)
                                        {
                                            ImGui::SetItemDefaultFocus();
                                        }
                                    }

                                    ImGui::EndCombo();
                                }

                                std::shared_ptr<ShadowMap> shadowMap = shadowCamera->GetShadowMap(selectedPointCubeDirection);

                                uint32_t shadowMapSize = shadowCamera->cameraWidth;
                                std::string shadowMapSizeStr = std::to_string(shadowMapSize);
                                if (ImGui::BeginCombo("Shadow Map Size", shadowMapSizeStr.c_str()))
                                {
                                    THETIS_SHADOWMAP_COMBO(128);
                                    THETIS_SHADOWMAP_COMBO(256);
                                    THETIS_SHADOWMAP_COMBO(512);
                                    THETIS_SHADOWMAP_COMBO(1024);
                                    THETIS_SHADOWMAP_COMBO(2048);
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
                                    THETIS_SHADOWMAP_COMBO(4096);
                                    THETIS_SHADOWMAP_COMBO(8192);
                                    // THETIS_SHADOWMAP_COMBO(16384);
                                    ImGui::PopStyleColor();

                                    ImGui::EndCombo();
                                }

                                AchillesImGui::Image(shadowMap, ImVec2(256.0f, 256.0f));
                            }

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
                            ImGui::Separator();
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

                            ImGui::Separator();
                            ImGui::DragFloat("Rank", &spotLight.Light.Rank, 0.25f, -25.0f, 25.0f, "%.0f");

                            // Debug
                            ImGui::Separator();
                            if (ImGui::Button("Set Debug ShadowCamera"))
                            {
                                Camera::debugShadowCamera = lightObject->GetShadowCamera(LightType::Spot, nullptr);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Clear DSC"))
                            {
                                Camera::debugShadowCamera = nullptr;
                            }

                            ImGui::Separator();
                            std::shared_ptr<ShadowCamera> shadowCamera = lightObject->GetShadowCamera(LightType::Spot, nullptr);
                            if (shadowCamera && shadowCamera->GetShadowMap() != nullptr)
                            {
                                std::shared_ptr<ShadowMap> shadowMap = shadowCamera->GetShadowMap();

                                uint32_t shadowMapSize = shadowCamera->cameraWidth;
                                std::string shadowMapSizeStr = std::to_string(shadowMapSize);
                                if (ImGui::BeginCombo("Shadow Map Size", shadowMapSizeStr.c_str()))
                                {
                                    THETIS_SHADOWMAP_COMBO(128);
                                    THETIS_SHADOWMAP_COMBO(256);
                                    THETIS_SHADOWMAP_COMBO(512);
                                    THETIS_SHADOWMAP_COMBO(1024);
                                    THETIS_SHADOWMAP_COMBO(2048);
                                    THETIS_SHADOWMAP_COMBO(4096);
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
                                    THETIS_SHADOWMAP_COMBO(8192);
                                    THETIS_SHADOWMAP_COMBO(16384);
                                    ImGui::PopStyleColor();

                                    ImGui::EndCombo();
                                }

                                AchillesImGui::Image(shadowMap, ImVec2(256.0f, 256.0f));
                            }

                            ImGui::TreePop();
                        }
                    }
                    if (lightObject->HasLightType(LightType::Directional))
                    {
                        if (ImGui::TreeNodeEx("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            static uint32_t selectedCascade = 0;

                            DirectionalLight& directionalLight = lightObject->GetDirectionalLight();
                            float color[3] =
                            {
                                directionalLight.Color.x,
                                directionalLight.Color.y,
                                directionalLight.Color.z,
                            };
                            if (ImGui::ColorEdit3("Color", color))
                            {
                                directionalLight.Color.x = color[0];
                                directionalLight.Color.y = color[1];
                                directionalLight.Color.z = color[2];
                            }

                            ImGui::DragFloat("Strength", &directionalLight.Strength, 0.025f, 0.0f, 100.0f, "%.3f");
                            ImGui::Separator();
                            ImGui::DragFloat("Rank", &directionalLight.Rank, 0.25f, -25.0f, 25.0f, "%.0f");

                            ImGui::Separator();

                            std::shared_ptr<ShadowCamera> shadowCamera = lightObject->GetShadowCamera(LightType::Directional, nullptr);

                            Matrix shadowMatrix = Matrix::Identity;
                            if (shadowCamera != nullptr)
                            {
                                std::vector<Matrix> shadowMatrices = shadowCamera->GetCascadeProjections();
                                if (selectedCascade < shadowMatrices.size() && shadowCamera->GetNumCascades() > 0)
                                    shadowMatrix = shadowCamera->GetView() * shadowMatrices[selectedCascade];

                                if (shadowCamera->GetNumCascades() <= 0)
                                {
                                    shadowMatrix = shadowCamera->GetView() * shadowCamera->GetProj();
                                }
                            }

                            ImGui::Text("Cascade Matrix");
                            ImGui::BeginDisabled(true);
                            ImGui::InputFloat4("##ShadowMatrix1", shadowMatrix.m[0], "%.3f", ImGuiInputTextFlags_ReadOnly);
                            ImGui::InputFloat4("##ShadowMatrix2", shadowMatrix.m[1], "%.3f", ImGuiInputTextFlags_ReadOnly);
                            ImGui::InputFloat4("##ShadowMatrix3", shadowMatrix.m[2], "%.3f", ImGuiInputTextFlags_ReadOnly);
                            ImGui::InputFloat4("##ShadowMatrix4", shadowMatrix.m[3], "%.3f", ImGuiInputTextFlags_ReadOnly);
                            ImGui::EndDisabled();

                            // Debug
                            ImGui::Separator();
                            if (ImGui::Button("Set Debug ShadowCamera"))
                            {
                                Camera::debugShadowCamera = shadowCamera;
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Clear DSC"))
                            {
                                Camera::debugShadowCamera = nullptr;
                            }
                            ImGui::Separator();

                            if (shadowCamera)
                            {
                                uint32_t numCascades = shadowCamera->GetNumCascades();
                                std::string numCascadesStr = std::to_string(numCascades);
                                if (ImGui::BeginCombo("Num Cascades", numCascadesStr.c_str()))
                                {
                                    for (uint32_t c = 0; c <= MAX_NUM_CASCADES; c++)
                                    {
                                        if (ImGui::Selectable(std::to_string(c).c_str(), numCascades == c))
                                        {
                                            postPresentFunctions.push_back([=]() { shadowCamera->ResizeCascades(c); });
                                        }
                                        if (numCascades == c)
                                        {
                                            ImGui::SetItemDefaultFocus();
                                        }
                                    }

                                    ImGui::EndCombo();
                                }

                                std::string selectedCascadeStr = std::to_string(selectedCascade);
                                if (ImGui::BeginCombo("View Cascade", selectedCascadeStr.c_str()))
                                {
                                    for (uint32_t c = 0; c < std::max<uint32_t>(1, shadowCamera->GetNumCascades()); c++)
                                    {
                                        if (ImGui::Selectable(std::to_string(c).c_str(), selectedCascade == c))
                                        {
                                            selectedCascade = c;
                                        }
                                        if (selectedCascade == c)
                                        {
                                            ImGui::SetItemDefaultFocus();
                                        }
                                    }

                                    ImGui::EndCombo();
                                }

                                if (numCascades > 0)
                                {
                                    float intervals[2] = { 0 };
                                    float cameraZRange = Camera::mainCamera->farZ - Camera::mainCamera->nearZ;

                                    if (selectedCascade == 0)
                                        intervals[0] = 0.0f;
                                    else
                                        intervals[0] = shadowCamera->GetCascadePartitions()[selectedCascade - 1];

                                    intervals[1] = std::max<float>(0.01f, shadowCamera->GetCascadePartitions()[selectedCascade]);

                                    ImGui::BeginDisabled(true);
                                    ImGui::InputFloat2("Partitions", intervals, "%.2f");

                                    intervals[0] *= cameraZRange;
                                    intervals[1] *= cameraZRange;

                                    ImGui::InputFloat2("Partition Intervals", intervals, "%.2f");
                                    ImGui::EndDisabled();
                                }

                                if (shadowCamera->GetShadowMap())
                                {
                                    ImGui::Separator();

                                    if (selectedCascade >= shadowCamera->GetNumCascades())
                                        selectedCascade = std::max<uint32_t>(1, shadowCamera->GetNumCascades()) - 1;
                                    if (shadowCamera->GetNumCascades() == 0)
                                        selectedCascade = 0;

                                    std::shared_ptr<ShadowMap> shadowMap = shadowCamera->GetShadowMap(selectedCascade);

                                    uint32_t shadowMapSize = shadowCamera->cameraWidth;
                                    std::string shadowMapSizeStr = std::to_string(shadowMapSize);
                                    if (ImGui::BeginCombo("Shadow Map Size", shadowMapSizeStr.c_str()))
                                    {
                                        THETIS_SHADOWMAP_COMBO(128);
                                        THETIS_SHADOWMAP_COMBO(256);
                                        THETIS_SHADOWMAP_COMBO(512);
                                        THETIS_SHADOWMAP_COMBO(1024);
                                        THETIS_SHADOWMAP_COMBO(2048);
                                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
                                        THETIS_SHADOWMAP_COMBO(4096);
                                        THETIS_SHADOWMAP_COMBO(8192);
                                        THETIS_SHADOWMAP_COMBO(16384);

                                        ImGui::PopStyleColor();

                                        ImGui::EndCombo();
                                    }

                                    ImGui::Separator();

                                    AchillesImGui::Image(shadowMap, ImVec2(256.0f, 256.0f));
                                }
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

// Temporary until we have object-bound cameras
void Thetis::DrawImGuiCameraProperties()
{
    if (ImGui::Begin("Camera Properties", &showCameraProperties))
    {
        ImGui::SetWindowSize(ImVec2(400, 600), ImGuiCond_Once);
        ImGui::SetWindowPos(ImVec2(clientWidth / 2.0f - 200.0f, clientHeight / 2.0f - 300.0f), ImGuiCond_Once);

        ImGui::DragFloat("Mouse Sensitivity", &mouseSensitivity, 0.05f, 0.0f, 5.0f, "%.3f");
        ImGui::DragFloat("Camera Base Move Speed", &cameraBaseMoveSpeed, 0.05f, 0.0f, 10.0f, "%.3f");

        std::shared_ptr<Camera> camera = Camera::debugShadowCamera == nullptr ? Camera::mainCamera : Camera::debugShadowCamera;
        if (camera == nullptr)
        {
            ImGui::Text("Main Camera was null");
        }
        else
        {
            ImGui::Text("Main Camera: %s", WStringToString(camera->name).c_str());
            float pos[3] = {
                    camera->GetPosition().x,
                    camera->GetPosition().y,
                    camera->GetPosition().z
            };
            if (ImGui::DragFloat3("Position", pos, 0.125f, 0.0, 0.0, "% .3f"))
            {
                camera->SetPosition(Vector3(pos[0], pos[1], pos[2]));
            }

            Vector3 degRot = EulerToDegrees(camera->GetRotation());
            float rot[3] = {
                degRot.x,
                degRot.y,
                degRot.z
            };
            if (ImGui::DragFloat3("Rotation", rot, 5.0f, -360, 360, "% .3f"))
            {
                camera->SetRotation(EulerToRadians(Vector3(rot[0], rot[1], rot[2])));
            }

            if (ImGui::DragFloat("NearZ", &camera->nearZ, 0.001f, 0.0001f, 10.0f, "%.4f"))
            {
                camera->ConstructProjection();
            }
            if (ImGui::DragFloat("FarZ", &camera->farZ, 0.1f, 0.01f, 10000.0f, "%.2f"))
            {
                camera->ConstructProjection();
            }

            ImGui::Separator();
            if (camera->IsOrthographic())
            {
                ImGui::Text("Orthographic");
                if (ImGui::Button("Set Perspective"))
                {
                    camera->SetPerspective(true);
                }

                if (ImGui::DragFloat("Orthographic Size", &camera->orthographicSize, 0.001f, 0.001f, 100.0f, "%.3f"))
                {
                    camera->ConstructProjection();
                }
            }
            else
            {
                ImGui::Text("Perspective");
                if (ImGui::Button("Set Orthographic"))
                {
                    camera->SetOrthographic(true);
                }

                float fov = camera->GetFOV();
                if (ImGui::DragFloat("Horizontal FOV", &fov, 0.5f, 0.5f, 125.0f, "%.1f"))
                {
                    camera->SetFOV(fov);
                }
            }

            ImGui::Separator();

            Matrix view = camera->GetView();
            ImGui::Text("View Matrix");
            bool viewChanged = ImGui::InputFloat4("##View1", view.m[0], "%.3f");
            viewChanged |= ImGui::InputFloat4("##View2", view.m[1], "%.3f");
            viewChanged |= ImGui::InputFloat4("##View3", view.m[2], "%.3f");
            viewChanged |= ImGui::InputFloat4("##View4", view.m[3], "%.3f");
            if (viewChanged)
            {
                camera->SetView(view);
            }

            Matrix proj = camera->GetProj();
            ImGui::Text("Projection Matrix");
            bool projChanged = ImGui::InputFloat4("##Proj1", proj.m[0], "%.3f");
            projChanged |= ImGui::InputFloat4("##Proj2", proj.m[1], "%.3f");
            projChanged |= ImGui::InputFloat4("##Proj3", proj.m[2], "%.3f");
            projChanged |= ImGui::InputFloat4("##Proj4", proj.m[3], "%.3f");
            if (projChanged)
            {
                camera->SetProj(proj);
            }

            Matrix projView = camera->GetView() * camera->GetProj();
            ImGui::Text("ProjView Matrix");
            ImGui::BeginDisabled(true);
            ImGui::InputFloat4("##ProjView1", projView.m[0], "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat4("##ProjView2", projView.m[1], "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat4("##ProjView3", projView.m[2], "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat4("##ProjView4", projView.m[3], "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
        }
    }
    ImGui::End();
}

void Thetis::DrawTextureSelection()
{
    if (ImGui::Begin("Texture Select", &selectingTexture))
    {
        ImVec2 windowSize = ImVec2(550, 550);
        ImGui::SetWindowSize(windowSize, ImGuiCond_Once);
        ImGui::SetWindowPos(ImVec2(clientWidth / 2.0f - (windowSize.x / 2.0f), clientHeight / 2.0f - (windowSize.y / 2.0f)), ImGuiCond_Once);

        auto textureNames = Texture::GetCachedTextureNames();

        static char textureSelectFilter[64];
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##Filter", "Filter", textureSelectFilter, 64);
        std::wstring filter = ToLowerWString(StringToWString(textureSelectFilter));

        if (ImGui::BeginTable("textureTable", 3, ImGuiTableFlags_BordersOuter))
        {
            ImGui::TableNextColumn();
            if (ImGui::Button("No Texture", ImVec2(128, 128)))
            {
                if (selectTextureCallback != nullptr)
                {
                    selectTextureCallback(nullptr);
                }
                selectingTexture = false;
            }

            for (size_t i = 0; i < textureNames.size(); i++)
            {
                std::wstring textureName = textureNames[i];
                std::shared_ptr<Texture> texture = Texture::GetCachedTexture(textureName);

                if (texture == nullptr)
                    continue;

                // If a filter is applied, check for the substring filter. If not found then continue
                if (filter != L"")
                {
                    if (ToLowerWString(texture->GetName()).find(filter) == std::string::npos)
                        continue;
                }

                float width = 0.0f;
                float height = 0.0f;
                if (!texture->GetSize(width, height))
                    continue; // continue if texture's resource doesn't exist

                ImVec2 size(128, 128);
                if (width > height)
                {
                    size.y = (height / width) * size.y;
                }
                else if (height > width)
                {
                    size.x = (width / height) * size.x;
                }

                ImGui::TableNextColumn();
                ImGui::PushID((int)i);
                if (AchillesImGui::ImageButton(texture, size, ImVec2(0, 0), ImVec2(1, 1), 2))
                {
                    if (selectTextureCallback != nullptr)
                    {
                        selectTextureCallback(texture);
                    }
                    selectingTexture = false;
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                    ImGui::SetTooltip(WStringToString(textureName).c_str());
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

    }
    ImGui::End();
}

void Thetis::SelectTexture(std::function<void(std::shared_ptr<Texture>)> callback)
{
    selectTextureCallback = callback;
    selectingTexture = true;
}

void Thetis::DrawImGuiPostProcessing()
{
    if (ImGui::Begin("Post Processing", &drawPostProcessing))
    {
        ImVec2 windowSize = ImVec2(300, 400);
        ImGui::SetWindowSize(windowSize, ImGuiCond_Once);
        ImGui::SetWindowPos(ImVec2(clientWidth / 2.0f - (windowSize.x / 2.0f), clientHeight / 2.0f - (windowSize.y / 2.0f)), ImGuiCond_Once);
        ImGui::Checkbox("Enable Post Processing", &postProcessingEnable);
        ImGui::Separator();
        if (postProcessing != nullptr)
        {
            ImGui::Checkbox("Enable Bloom", &postProcessing->EnableBloom);
            ImGui::DragFloat("Bloom Strength", &postProcessing->BloomStrength, 0.01f, 0.0f, 2.0f, "%.2f");
            ImGui::DragFloat("Bloom Threshold", &postProcessing->BloomThreshold, 0.1f, 0.0f, 8.0f, "%.2f");
            ImGui::DragFloat("Bloom Upsample Factor", &postProcessing->BloomUpsampleFactor, 0.01f, 0.0f, 2.0f, "%.2f");

            ImGui::Separator();

            ImGui::Checkbox("Enable Tone Mapping", &postProcessing->EnableToneMapping);
            if (ImGui::BeginCombo("Tone Mapper", PPToneMapping::ToneMappers::GetToneMapperName(postProcessing->ToneMapper).c_str()))
            {
                THETIS_TONEMAPPER_COMBO(Clamp);
                THETIS_TONEMAPPER_COMBO(ExtendedReinhard);
                THETIS_TONEMAPPER_COMBO(Filmic);

                ImGui::EndCombo();
            }

            ImGui::Separator();

            ImGui::Checkbox("Enable Gamma Correction", &postProcessing->EnableGammaCorrection);
            ImGui::DragFloat("Gamma Correction", &postProcessing->GammaCorrection, 0.05f, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        }
    }
    ImGui::End();
}

void Thetis::DrawImGuiSkyboxProperties()
{
    if (ImGui::Begin("Skybox", &drawSkyboxProperties))
    {
        ImVec2 windowSize = ImVec2(300, 550);
        ImGui::SetWindowSize(windowSize, ImGuiCond_Once);
        ImGui::SetWindowPos(ImVec2(clientWidth / 2.0f - (windowSize.x / 2.0f), clientHeight / 2.0f - (windowSize.y / 2.0f)), ImGuiCond_Once);

        if (skydome != nullptr)
        {
            bool skyboxActive = skydome->IsActive();
            if (ImGui::Checkbox("Skybox Active", &skyboxActive))
            {
                skydome->SetActive(skyboxActive);
            }

            ImGui::Separator();

            Material& skyMaterial = skydome->GetMaterial(0);

            Color skyColor = skyMaterial.GetVector(L"SkyColor");
            if (ImGui::ColorEdit3("Sky Color", &skyColor.x))
                skyMaterial.SetVector(L"SkyColor", skyColor);

            Color upSkyColor = skyMaterial.GetVector(L"UpSkyColor");
            if (ImGui::ColorEdit3("Up Sky Color", &upSkyColor.x))
                skyMaterial.SetVector(L"UpSkyColor", upSkyColor);

            Color horizonColor = skyMaterial.GetVector(L"HorizonColor");
            if (ImGui::ColorEdit3("Horizon Color", &horizonColor.x))
                skyMaterial.SetVector(L"HorizonColor", horizonColor);

            Color groundColor = skyMaterial.GetVector(L"GroundColor");
            if (ImGui::ColorEdit3("Ground Color", &groundColor.x))
                skyMaterial.SetVector(L"GroundColor", groundColor);

            ImGui::Separator();

            float primarySunSize = skyMaterial.GetFloat(L"PrimarySunSize");
            if (ImGui::DragFloat("Sun Size", &primarySunSize, 0.05f, 0.0f, 10.0f, "%.2f"))
                skyMaterial.SetFloat(L"PrimarySunSize", primarySunSize);

            float primarySunShineExponent = skyMaterial.GetFloat(L"PrimarySunShineExponent");
            if (ImGui::DragFloat("Primary Sun Shine Exponent", &primarySunShineExponent, 0.1f, 0.0f, 64.0f, "%.1f"))
                skyMaterial.SetFloat(L"PrimarySunShineExponent", primarySunShineExponent);

            bool debugCheck = skyMaterial.GetFloat(L"Debug") > 0.5f;
            if (ImGui::Checkbox("Debug", &debugCheck))
            {
                skyMaterial.SetFloat(L"Debug", debugCheck ? 1.0f : 0.0f);
            }

            bool hideSunBehindHorizon = skyMaterial.GetFloat(L"HideSunBehindHorizon");
            if (ImGui::Checkbox("Hide Sun Behind Horizon", &hideSunBehindHorizon))
            {
                skyMaterial.SetFloat(L"HideSunBehindHorizon", hideSunBehindHorizon ? 1.0f : 0.0f);
            }

            // Cubemap Texture
            ImGui::Text("Cubemap Texture");
            std::shared_ptr<Texture> cubemap = skyMaterial.GetTexture(L"Cubemap");

            if (cubemap == nullptr)
            {
                if (ImGui::Button("No Texture", ImVec2(256, 256)))
                {
                    SelectTexture([&](std::shared_ptr<Texture> newTexture) { skyMaterial.SetTexture(L"Cubemap", newTexture); });
                }
            }
            else
            {
                float width = 0.0f;
                float height = 0.0f;
                cubemap->GetSize(width, height);

                ImVec2 size(256, 256);
                if (width > height)
                    size.y = (height / width) * size.y;
                else if (height > width)
                    size.x = (width / height) * size.x;

                if (AchillesImGui::ImageButton(cubemap, size, ImVec2(0, 0), ImVec2(1, 1), 2))
                {
                    SelectTexture([&](std::shared_ptr<Texture> newTexture) { skyMaterial.SetTexture(L"Cubemap", newTexture); });
                }
            }
        }
    }
    ImGui::End();
}

void Thetis::OnPostRender(float deltaTime)
{
    ScopedTimer _prof(L"OnPostRender");
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
    if (showCameraProperties)
    {
        DrawImGuiCameraProperties();
    }
    if (selectingTexture)
    {
        DrawTextureSelection();
    }
    if (drawPostProcessing)
    {
        DrawImGuiPostProcessing();
    }
    if (drawSkyboxProperties)
    {
        DrawImGuiSkyboxProperties();
    }
}

void Thetis::OnResize(int newWidth, int newHeight)
{
    if (camera != nullptr)
        camera->UpdateViewport(newWidth, newHeight);
}

void Thetis::LoadContent()
{
    ACHILLES_IF_DESTROYING_RETURN();

    // Set editor to true so we draw editor sprites
    Application::SetIsEditor(true);

    // Get command queue + list
    std::shared_ptr<CommandQueue> commandQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

    std::shared_ptr<CommandQueue> computeQueue = GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    std::shared_ptr<CommandList> computeList = computeQueue->GetCommandList();

    // Create camera
    camera = std::make_shared<Camera>(L"Thetis Camera", clientWidth, clientHeight);
    Camera::mainCamera = camera;
    camera->SetFOV(90.0f);
    camera->SetPosition(Vector3(0, 0, -5));
    // camera->SetRotation(EulerToRadians(Vector3(0, 0, 0)));

    // Create shaders
    std::shared_ptr<Shader> posColShader = GetPosColShader(device);
    std::shared_ptr<Shader> posTexturedShader = PosTextured::GetPosTexturedShader(device);
    std::shared_ptr<Shader> blinnPhongShader = BlinnPhong::GetBlinnPhongShader(device);

    std::shared_ptr<Shader> waterShader = Water::GetWaterShader(device);

    // Create floor quad
    std::shared_ptr<Object> floorQuad = Object::CreateObjectsFromContentFile(L"plane.fbx", blinnPhongShader);
    floorQuad->SetLocalPosition(Vector3(0, -3, 0));
    floorQuad->SetLocalScale(Vector3(10, 10, 10));

    std::shared_ptr<Texture> floorTexture = Texture::AddCachedTextureFromContent(commandList, L"MyUVSquare");
    floorQuad->GetMaterial().SetTexture(L"MainTexture", floorTexture);
    mainScene->AddObjectToScene(floorQuad);

    std::shared_ptr<Object> achillesTest2 = Object::CreateObjectsFromContentFile(L"achilles test 2.fbx", blinnPhongShader);
    std::shared_ptr<Object> testLight = achillesTest2->FindFirstObjectByName(L"Light");
    if (testLight)
        testLight->SetActive(false);
    mainScene->AddObjectToScene(achillesTest2);

    ACHILLES_IF_DESTROYING_RETURN();

    std::shared_ptr<LightObject> spotLightObject = std::make_shared<LightObject>(L"Spotlight");
    SpotLight spotLight = SpotLight();
    spotLight.InnerSpotAngle = toRad(35.0f);
    spotLight.OuterSpotAngle = toRad(40.0f);
    spotLightObject->AddLight(spotLight);
    spotLightObject->SetLocalPosition(Vector3(0, 7.5f, 0));
    spotLightObject->SetLocalRotation(Quaternion(0.0f, -1.0f, 0.0f, 0.0f));
    spotLightObject->SetActive(false);
    spotLightObject->SetIsShadowCaster(true);
    mainScene->AddObjectToScene(spotLightObject);

    std::shared_ptr<LightObject> sunLightObject = std::make_shared<LightObject>(L"Sun");
    sunLightObject->AddLight(DirectionalLight());
    sunLightObject->SetLocalPosition(Vector3(0, 7.5f, 0));
    sunLightObject->SetLocalRotation(Quaternion(0.0f, -1.0f, 0.0f, 0.0f));
    sunLightObject->SetActive(false);
    sunLightObject->SetIsShadowCaster(true);
    mainScene->AddObjectToScene(sunLightObject);

    std::shared_ptr<LightObject> pointLightObject = std::make_shared<LightObject>(L"Pointlight");
    pointLightObject->AddLight(PointLight());
    pointLightObject->SetLocalPosition(Vector3(0, 2.0f, 0));
    pointLightObject->SetLocalRotation(Quaternion::Identity);
    pointLightObject->SetActive(true);
    pointLightObject->SetIsShadowCaster(true);
    mainScene->AddObjectToScene(pointLightObject);

    // Secondary scene
    {
        std::shared_ptr<Scene> scene2 = std::make_shared<Scene>(L"Spotlights");
        AddScene(scene2);
        scene2->SetActive(false);

        SpotLight spot = SpotLight();
        spot.InnerSpotAngle = toRad(42.5);
        spot.OuterSpotAngle = toRad(47.5);
        std::shared_ptr<LightObject> spotR = std::make_shared<LightObject>(L"Spot Red");
        spot.Light.Color = Color(1, 0, 0, 1);
        spotR->AddLight(spot);
        spotR->SetLocalPosition(Vector3(2.0f, 7.5f, 0));
        spotR->SetLocalRotation(Quaternion(0.0f, -1.0f, 0.0f, 0.0f));
        spotR->SetIsShadowCaster(true);
        scene2->AddObjectToScene(spotR);

        std::shared_ptr<LightObject> spotG = std::make_shared<LightObject>(L"Spot Green");
        spot.Light.Color = Color(0, 1, 0, 1);
        spotG->AddLight(spot);
        spotG->SetLocalPosition(Vector3(-1.0f, 7.5f, 1.75f));
        spotG->SetLocalRotation(Quaternion(0.0f, -1.0f, 0.0f, 0.0f));
        spotG->SetIsShadowCaster(true);
        scene2->AddObjectToScene(spotG);

        std::shared_ptr<LightObject> spotB = std::make_shared<LightObject>(L"Spot Blue");
        spot.Light.Color = Color(0, 0, 1, 1);
        spotB->AddLight(spot);
        spotB->SetLocalPosition(Vector3(-1.0f, 7.5f, -1.75f));
        spotB->SetLocalRotation(Quaternion(0.0f, -1.0f, 0.0f, 0.0f));
        spotB->SetIsShadowCaster(true);
        scene2->AddObjectToScene(spotB);

        std::shared_ptr<Object> quad2 = floorQuad->Clone();
        quad2->SetName(L"Spotlight Quad");
        quad2->SetLocalScale(Vector3(15.0f, 15.0f, 15.0f));
        quad2->GetMaterial(0).SetTexture(L"MainTexture", nullptr);
        scene2->AddObjectToScene(quad2);

        std::shared_ptr<Object> torus = Object::CreateObjectsFromContentFile(L"torus.fbx", BlinnPhong::GetBlinnPhongShader(device));
        torus->SetLocalScale(Vector3(2.0f, 2.0f, 2.0f));
        scene2->AddObjectToScene(torus);
    }

    // Load content/models files into meshNames, used in Thetis' ImGui
    PopulateMeshNames();
    // Load and cache some extra textures we might use at runtime
    Texture::AddCachedTextureFromContent(commandList, L"ColorGrid");
    Texture::AddCachedTextureFromContent(commandList, L"UVGrid", TextureUsage::Linear);
    Texture::AddCachedTextureFromContent(commandList, L"Icy Planet Diffuse");
    Texture::AddCachedTextureFromContent(commandList, L"Icy Planet Normal", TextureUsage::Normalmap);

    // Load Outdoor HDRI 064 HDRI from Pano to Cubemap
    std::shared_ptr<Texture> outdoorPano = std::make_shared<Texture>(TextureUsage::Linear, L"Outdoor HDRI 064 Pano");
    commandList->LoadTextureFromContent(*outdoorPano, L"OutdoorHDRI064_4K-TONEMAPPED", TextureUsage::Linear);
    std::shared_ptr<Texture> outdoorCubemap = CreateCubemap(1024, 1024);
    outdoorCubemap->SetName(L"Outdoor HDRI 64 Cubemap");
    computeList->PanoToCubemap(*outdoorCubemap, *outdoorPano);
    // Set the skybox's cubemap texture
    skydome->GetMaterial().SetTexture(L"Cubemap", outdoorCubemap);
    Texture::AddCachedTexture(L"Outdoor HDRI 64 Cubemap", outdoorCubemap);

    // Create post processing class
    postProcessing = std::make_shared<PostProcessing>((float)clientWidth, (float)clientHeight);


    // Create debug wireframe bounding box
    debugBoundingBox = Object::CreateObjectsFromContentFile(L"cube.fbx", DebugWireframe::GetDebugWireframeShader(device));
    debugBoundingBox->GetMaterial(0).SetVector(L"Color", Color(1.0f, 0.0f, 0.0f, 1.0f)); // red

    debugBoundingBoxCenter = Object::CreateObjectsFromContentFile(L"icosphere.fbx", DebugWireframe::GetDebugWireframeShader(device));
    debugBoundingBoxCenter->GetMaterial(0).SetVector(L"Color", Color(0.0f, 1.0f, 0.0f, 1.0f)); // green

    debugBoundingBoxAABB = debugBoundingBox->Clone();
    debugBoundingBoxAABB->GetMaterial(0).SetVector(L"Color", Color(0.0f, 0.0f, 1.0f, 1.0f)); // blue

    ACHILLES_IF_DESTROYING_RETURN();

    // Execute direct command list
    uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    // Execute compute command list
    fenceValue = computeQueue->ExecuteCommandList(computeList);
    computeQueue->WaitForFenceValue(fenceValue);
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
    if (kb.LeftShift && kbt.pressed.F5)
    {
        showCameraProperties = !showCameraProperties;
    }
    if (kb.LeftControl && kbt.pressed.F5)
    {
        drawPostProcessing = !drawPostProcessing;
    }
    if (kb.LeftControl && kbt.pressed.F6)
    {
        drawSkyboxProperties = !drawSkyboxProperties;
    }

    if (kbt.pressed.PageUp)
    {
        camera->SetFOV(camera->GetFOV() - 5);
        OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
    }
    if (kbt.pressed.PageDown)
    {
        camera->SetFOV(camera->GetFOV() + 5);
        OutputDebugStringWFormatted(L"FOV is now %.1f\n", camera->fov);
    }

    float cameraRotationAmount = 45 * dt;
    if (kb.LeftControl)
        cameraRotationAmount /= 4.0f;
    if (kb.LeftShift)
        cameraRotationAmount *= 4.0f;
    if (kb.Up)
    {
        camera->RotateEuler(Vector3(-toRad(cameraRotationAmount), 0, 0));
    }
    if (kb.Down)
    {
        camera->RotateEuler(Vector3(toRad(cameraRotationAmount), 0, 0));
    }
    if (kb.Right)
    {
        camera->RotateEuler(Vector3(0, toRad(cameraRotationAmount), 0));
    }
    if (kb.Left)
    {
        camera->RotateEuler(Vector3(0, -toRad(cameraRotationAmount), 0));
    }

    float movementSpeed = cameraBaseMoveSpeed;
    if (kb.LeftControl)
        movementSpeed /= 4.0f;
    if (kb.LeftShift)
        movementSpeed *= 4.0f;

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

    if (kbt.pressed.Delete)
    {
        DeleteSelectedObject();
    }
    if (kbt.pressed.T)
    {
        if (kb.LeftControl)
        {
            if (selectedPropertiesScene != nullptr)
                selectedPropertiesScene->SetActive(!selectedPropertiesScene->IsActive());
        }
        else
        {
            if (selectedPropertiesObject != nullptr)
                selectedPropertiesObject->SetActive(!selectedPropertiesObject->IsActive());
        }
    }

    // Previously in Achilles HandleKeyboard
    if (kbt.pressed.Escape)
    {
        if (selectingTexture) // Close Texture select before skybox properties
            selectingTexture = false;
        else if (drawSkyboxProperties) // Close skybox properties before post processing properties
            drawSkyboxProperties = false;
        else if (drawPostProcessing) // Close post processing properties before camera popup
            drawPostProcessing = false;
        else if (showCameraProperties) // Close camera popup before application closing
            showCameraProperties = false;
        else // Close application on ESC hit
            PostQuitMessage(0);
    }

    if (kbt.pressed.V)
    {
        vSync = !vSync;
    }

    // Press B to toggle bloom
    if (kbt.pressed.B)
    {
        if (postProcessing != nullptr)
        {
            postProcessing->EnableBloom = !postProcessing->EnableBloom;
        }
    }

    if (kbt.pressed.P)
    {
        Profiling::ProfilerShouldPrint = true;
    }

    if (kb.LeftControl && kbt.pressed.F8)
    {
        Application::SetIsEditor(!Application::IsEditor());
        OutputDebugStringW(Application::IsEditor() ? L"Application::IsEditor\n" : L"!Application::IsEditor\n");
    }
}

void Thetis::OnMouse(Mouse::ButtonStateTracker mt, MouseData md, Mouse::State state, float dt)
{
    if (mt.rightButton)
    {
        float cameraRotationSpeed = toRad(45) * 0.01f * mouseSensitivity;
        camera->RotateEuler(Vector3(cameraRotationSpeed * md.mouseYDelta, cameraRotationSpeed * md.mouseXDelta, 0));
    }
    if (md.scrollDelta < 0)
    {
        if (cameraBaseMoveSpeed <= 1)
            cameraBaseMoveSpeed -= 0.125;
        else
            cameraBaseMoveSpeed -= 0.5;

        if (cameraBaseMoveSpeed < 0.125)
            cameraBaseMoveSpeed = 0.125;
    }
    else if (md.scrollDelta > 0)
    {
        if (cameraBaseMoveSpeed < 1)
            cameraBaseMoveSpeed += 0.125;
        else
            cameraBaseMoveSpeed += 0.5;

        if (cameraBaseMoveSpeed > 10)
            cameraBaseMoveSpeed = 10;
    }

    if (mt.leftButton == mt.HELD)
    {

    }
    else if (mt.leftButton == mt.RELEASED)
    {
        selectedPropertiesObject = PickObject(md.mouseX, md.mouseY);
        if (selectedPropertiesObject != nullptr)
            selectedPropertiesScene = selectedPropertiesObject->GetScene();
    }
}

void Thetis::OnGamePad(float dt)
{
    auto state = gamepad->GetState(0, GamePad::DEAD_ZONE_CIRCULAR);

    if (state.IsConnected())
    {
        float leftX = state.thumbSticks.leftX;
        float leftY = state.thumbSticks.leftY;

        float rightX = state.thumbSticks.rightX;
        float rightY = state.thumbSticks.rightY;

        float cameraRotationAmount = 45 * 3.0f * dt;
        camera->RotateEuler(Vector3(toRad(cameraRotationAmount) * -rightY, toRad(cameraRotationAmount) * rightX, 0));

        float movementSpeed = cameraBaseMoveSpeed * 2.0f * dt;
        camera->MoveRelative(Vector3(movementSpeed * leftX, 0, movementSpeed * leftY));

        if (state.IsRightShoulderPressed())
            camera->MoveRelative(Vector3(0, movementSpeed, 0));

        if (state.IsLeftShoulderPressed())
            camera->MoveRelative(Vector3(0, -movementSpeed, 0));
    }
}

void Thetis::AddObjectToScene(std::shared_ptr<Object> object)
{
    if (selectedPropertiesScene == nullptr)
        return;
    selectedPropertiesScene->AddObjectToScene(object);
    selectedPropertiesObject = object;
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

void Thetis::CreateObjectInSelectedScene(uint32_t meshNameIndex)
{
    if (selectedPropertiesScene == nullptr)
        return;

    std::shared_ptr<Object> object = Object::CreateObjectsFromContentFile(meshNamesWide[meshNameIndex], BlinnPhong::GetBlinnPhongShader(device));
    selectedPropertiesScene->AddObjectToScene(object);
    selectedPropertiesObject = object;
}

void Thetis::CreateObjectAsSelectedChild(uint32_t meshNameIndex)
{
    if (selectedPropertiesObject == nullptr)
    {
        CreateObjectInSelectedScene(meshNameIndex);
        return;
    }
    std::shared_ptr<Object> object = Object::CreateObjectsFromContentFile(meshNamesWide[meshNameIndex], BlinnPhong::GetBlinnPhongShader(device));
    selectedPropertiesObject->AddChild(object);
    selectedPropertiesObject = object;
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

void Thetis::ClearSelectedParent()
{
    if (selectedPropertiesObject == nullptr)
        return;

    std::shared_ptr<Scene> currentScene = mainScene;
    if (selectedPropertiesObject->GetScene() != nullptr)
        currentScene = selectedPropertiesObject->GetScene();

    selectedPropertiesObject->SetParentKeepTransform(currentScene->GetObjectTree());
}

void Thetis::AddWaterPlane()
{
    std::shared_ptr<Object> plane = Object::CreateObjectsFromContentFile(L"subdiv plane.fbx", BlinnPhong::GetBlinnPhongShader(device));
    if (plane != nullptr)
    {
        plane->GetMaterial(0).shader = Water::GetWaterShader(device);
        plane->SetName(L"Water Plane");
        plane->SetLocalScale(Vector3(1, 1, 1));
        plane->SetCastsShadows(false);

        selectedPropertiesScene->AddObjectToScene(plane);
        selectedPropertiesObject = plane;
    }
}
