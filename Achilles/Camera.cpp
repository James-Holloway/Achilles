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
    if (dirtyViewMatrix || dirtyProjMatrix)
        ConstructMatrices();
    return view * proj;
}

Matrix Camera::GetView()
{
    if (dirtyViewMatrix)
        ConstructView();
    return view;
}

Matrix Camera::GetProj()
{
    if (dirtyProjMatrix)
        ConstructProjection();
    return proj;
}

void Camera::ConstructMatrices()
{
    ConstructView();
    ConstructProjection();
}

Matrix Camera::GetInverseView()
{
    if (dirtyViewMatrix)
        ConstructView();
    return inverseView;
}

void Camera::ConstructView()
{
    view = Matrix::CreateFromYawPitchRoll(rotation) * Matrix::CreateTranslation(position);
    inverseView = view.Transpose();
    view = view.Invert();
    dirtyViewMatrix = false;
}

void Camera::ConstructProjection()
{
    proj = PerspectiveFovProjection(viewport.Width, viewport.Height, fov, nearZ, farZ);
    dirtyProjMatrix = false;
}


void Camera::RotateEuler(Vector3 euler, bool unlockPitch, bool unlockRoll)
{
    Vector3 rot = rotation;
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
    dirtyViewMatrix = true;
}

void Camera::MoveRelative(Vector3 direction)
{
    Matrix world = Matrix(Vector3::Right, Vector3::Up, -Vector3::Forward);
    Matrix rot = Matrix::CreateFromYawPitchRoll(rotation);
    Vector3 pos = DirectX::XMVector3TransformNormal(direction, world * rot);
    position += pos;
    dirtyViewMatrix = true;
}
