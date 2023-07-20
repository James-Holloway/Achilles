#pragma once
#include "Achilles/Achilles.h"
#pragma warning (push)
#pragma warning (disable : 26451)
#include "imgui.h"
#include "implot.h"
#pragma warning (pop)
#include "Achilles/AchillesShaders.h"

class Thetis : public Achilles
{
protected:
    std::shared_ptr<Object> floorQuad;
    std::shared_ptr<Camera> camera;

    float mouseSensitivity = 1.0f;
    float cameraBaseMoveSpeed = 4.0f;

    bool showPerformance = true;
    bool showObjectTree = true;
    bool showProperties = true;
    bool showCameraProperties = false;

    std::vector<std::string> meshNames;
    std::vector<std::wstring> meshNamesWide;
    int selectedMeshName = 0;
public:
    inline static std::shared_ptr<Object> selectedPropertiesObject = nullptr;

public:
    Thetis(std::wstring name);
public:
    virtual void OnUpdate(float deltaTime) override; // Post internal Update
    virtual void OnRender(float deltaTime) override;
    // Called after render + depth clear
    virtual void OnPostRender(float deltaTime) override; // Just before internal Present
    virtual void OnResize(int newWidth, int newHeight) override; // Post resize
    virtual void LoadContent() override; // Load content to be used in Render
    virtual void UnloadContent() override; // Unload content on quit
    virtual void OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt) override;
    virtual void OnMouse(DirectX::Mouse::ButtonStateTracker mt, MouseData md, DirectX::Mouse::State state, float dt) override;

protected:
    void DrawImGuiPerformance();
    void DrawImGuiScenes();
    void DrawImGuiProperties();
    void DrawImGuiCameraProperties();

    void PopulateMeshNames();
    void CreateObjectInMainScene(uint32_t meshNameIndex);
    void CreateObjectAsSelectedChild(uint32_t meshNameIndex);
    void DeleteSelectedObject();
    std::shared_ptr<Object> CopySelectedObject();
};