#pragma once
#include "Achilles/Achilles.h"
#pragma warning (push)
#pragma warning (disable : 26451)
#include "imgui.h"
#include "implot.h"
#pragma warning (pop)

class Thetis : public Achilles
{
protected:
    std::shared_ptr<Camera> camera;

    float mouseSensitivity = 1.0f;
    float cameraBaseMoveSpeed = 4.0f;

    bool showPerformance = true;
    bool showObjectTree = true;
    bool showProperties = true;
    bool showCameraProperties = false;
    bool drawPostProcessing = false;
    bool drawSkyboxProperties = false;

    bool selectingTexture = false;
    std::function<void(std::shared_ptr<Texture>)> selectTextureCallback;

    std::vector<std::string> meshNames;
    std::vector<std::wstring> meshNamesWide;
    int selectedMeshName = 0;

    std::shared_ptr<Object> debugBoundingBox;
    std::shared_ptr<Object> debugBoundingBoxCenter;
    bool drawDebugBoundingBox = false;

public:
    inline static std::shared_ptr<Object> selectedPropertiesObject = nullptr;
    inline static std::shared_ptr<Scene> selectedPropertiesScene = nullptr;

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

    virtual void AddObjectToScene(std::shared_ptr<Object> object) override;

protected:
    void DrawImGuiTextureProperty(std::shared_ptr<Object> object, uint32_t knitIndex, std::wstring name);
    void DrawImGuiPerformance();
    void DrawImGuiScenes();
    void DrawImGuiProperties();
    void DrawImGuiCameraProperties();

    void DrawTextureSelection();
    void SelectTexture(std::function<void(std::shared_ptr<Texture>)> callback);

    void DrawImGuiPostProcessing();
    void DrawImGuiSkyboxProperties();

    void PopulateMeshNames();
    void CreateObjectInSelectedScene(uint32_t meshNameIndex);
    void CreateObjectAsSelectedChild(uint32_t meshNameIndex);
    void DeleteSelectedObject();
    std::shared_ptr<Object> CopySelectedObject();
    void ClearSelectedParent();
};