#include "Camera.h"
using namespace DirectX;
using namespace DirectX::SimpleMath;

Camera::Camera(std::wstring _name, int width, int height)
{
	name = _name;
	UpdateViewport(width, height);
}

void Camera::UpdateViewport(int width, int height)
{
	viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
}

Matrix Camera::GetViewProj()
{
	ConstructMatrices();
	return view * proj;
}

Matrix Camera::GetView()
{
	ConstructView();
	return view;
}

Matrix Camera::GetProj()
{
	ConstructProjection();
	return proj;
}

void Camera::ConstructMatrices()
{
	ConstructView();
	ConstructProjection();
}

void Camera::ConstructView()
{
	view = Matrix::CreateFromYawPitchRoll(rotation) * Matrix::CreateTranslation(position);
	view = view.Invert();
	// view = view.Transpose();
	// view = ViewFPS(position, 0, 0);
}

void Camera::ConstructProjection()
{
	proj = PerspectiveFovProjection(viewport.Width, viewport.Height, fov, nearZ, farZ);
}


void Camera::RotateEuler(Vector3 euler, bool unlockPitch, bool unlockRoll)
{
	Vector3 rot = rotation;
	Vector3 deg = EulerToDegrees(rot);
	OutputDebugStringWFormatted(L"Euler: % .2f % .2f % .2f\n", euler.x, euler.y, euler.z);
	OutputDebugStringWFormatted(L"Camera Rotation Before: % .2f % .2f % .2f\n", deg.x, deg.y, deg.z);
	rot += euler;
	rot = EulerVectorModulo(rot);
	if (!unlockPitch)
	{
		rot.x = fmin(AchillesPi, rot.x);
		rot.x = fmax(-AchillesPi, rot.x);
	}
	if (!unlockRoll)
		rot = Vector3(rot.x, rot.y, 0);

	rotation = rot;
	deg = EulerToDegrees(rotation);
	OutputDebugStringWFormatted(L"Camera Rotation: % .2f % .2f % .2f\n", deg.x, deg.y, deg.z);
}

void Camera::MoveRelative(Vector3 direction)
{
	Matrix world = Matrix(Vector3::Right, Vector3::Up, -Vector3::Forward);
	Matrix rot = Matrix::CreateFromYawPitchRoll(rotation);
	Vector3 pos = DirectX::XMVector3TransformNormal(direction, world * rot);
	position += pos;
	OutputDebugStringWFormatted(L"Camera Position: % .2f % .2f % .2f\n", position.x, position.y, position.z);
}
