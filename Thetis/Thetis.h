#pragma once
#include "Achilles.h"

class Thetis : public Achilles
{
public:
	Thetis(std::wstring name);
public:
	virtual void OnUpdate(float deltaTime) override; // Post internal Update
	virtual void OnRender(float deltaTime) override; // Called after render + depth clear
	virtual void OnPostRender(float deltaTime) override; // Just before internal Present
	virtual void OntResize(int newWidth, int newHeight) override; // Post resize
	virtual void LoadContent() override; // Load content to be used in Render
	virtual void UnloadContent() override; // Unload content on quit
	virtual void OnKeyboard(Keyboard::KeyboardStateTracker kbt) override;
	virtual void OnMouse(Mouse::ButtonStateTracker mt) override;
};