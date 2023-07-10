#pragma once
#include "Achilles/Achilles.h"
#pragma warning (push)
#pragma warning (disable : 26451)
#include "imgui.h"
#include "implot.h"
#pragma warning (pop)
#include "PosCol.h"
#include "PosTextured.h"

class Thetis : public Achilles
{
protected:
	Object* cube;
	Object* floorQuad;
	std::shared_ptr<Camera> camera;

	float cubeRotationSpeed = 2.0f;
	float mouseSensitivity = 1.0f;
	float cameraBaseMoveSpeed = 4.0f;

	bool showPerformance = false;

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