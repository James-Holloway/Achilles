#pragma once
#include "Achilles/Achilles.h"
#include "PosCol.h"

class Thetis : public Achilles
{
protected:
	std::shared_ptr<Mesh> cube;
	std::shared_ptr<Camera> camera;

	float cubeRotationSpeed = 2.0f;
	float mouseSensitivity = 1.0f;
	bool lockMouseToWindow = true;

public:
	Thetis(std::wstring name);
public:
	virtual void OnUpdate(float deltaTime) override; // Post internal Update
	virtual void OnRender(float deltaTime) override; // Called after render + depth clear
	virtual void OnPostRender(float deltaTime) override; // Just before internal Present
	virtual void OnResize(int newWidth, int newHeight) override; // Post resize
	virtual void LoadContent() override; // Load content to be used in Render
	virtual void UnloadContent() override; // Unload content on quit
	virtual void OnKeyboard(DirectX::Keyboard::KeyboardStateTracker kbt, DirectX::Keyboard::Keyboard::State kb, float dt) override;
	virtual void OnMouse(DirectX::Mouse::ButtonStateTracker mt, MouseData md, DirectX::Mouse::State state, float dt) override;
};