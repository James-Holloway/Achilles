#pragma once
#include "Common.h"

class Camera
{
	friend class Achilles;
public:
	// Statics
	inline static std::shared_ptr<Camera> mainCamera{};
public:
	// Members
	std::wstring name;

	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Vector3 rotation;

	float fov = 60.0f; // vertical FOV - 60deg vertical ~= 90deg horizontal
	float nearZ = 0.1f;
	float farZ = 100.0f;
	CD3DX12_RECT scissorRect{ 0, 0, LONG_MAX, LONG_MAX };
	CD3DX12_VIEWPORT viewport;
	DirectX::SimpleMath::Matrix view;
	DirectX::SimpleMath::Matrix proj;
public:
	Camera(std::wstring _name, int width, int height);
	void UpdateViewport(int width, int height);

	DirectX::SimpleMath::Matrix GetView();
	DirectX::SimpleMath::Matrix GetProj();
	DirectX::SimpleMath::Matrix GetViewProj();

	void ConstructMatrices();
	void ConstructView();
	void ConstructProjection();

	void RotateEuler(Vector3 euler, bool unlockPitch = false, bool unlockRoll = false);
	void MoveRelative(Vector3 direction);
};