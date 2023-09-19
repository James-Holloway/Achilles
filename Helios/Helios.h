#pragma once
#include "Achilles/Achilles.h"
#pragma warning (push)
#pragma warning (disable : 26451)
#include "imgui.h"
#include "implot.h"
#pragma warning (pop)
#include "Spaceship.h"

class Helios : public Achilles
{
protected:
    std::shared_ptr<Spaceship> playerShip;
    std::shared_ptr<LightObject> sun;

    bool showDebug = true;

public:
    Helios(std::wstring _name = L"Helios", uint32_t width = 1600, uint32_t height = 900);

protected:
    virtual void OnUpdate(float deltaTime) override; // Post internal Update
    virtual void OnPostRender(float deltaTime) override; // Just before internal Present
    virtual void OnResize(int newWidth, int newHeight) override; // Post resize
    virtual void LoadContent() override; // Load content to be used in Render
    virtual void OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt) override;
    virtual void OnMouse(DirectX::Mouse::ButtonStateTracker mt, MouseData md, DirectX::Mouse::State state, float dt) override;
};