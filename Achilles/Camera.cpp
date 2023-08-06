#include "Camera.h"
#include "MathHelpers.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Camera::Camera(std::wstring _name, int width, int height)
{
    name = _name;
    UpdateViewport(width, height);
}

Camera::~Camera()
{

}

void Camera::UpdateViewport(int width, int height)
{
    viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
    dirtyProjMatrix = true;
}

Matrix Camera::GetViewProj()
{
    if (dirtyViewMatrix)
        ConstructView();
    if (dirtyProjMatrix)
        ConstructProjection();
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

Matrix Camera::GetInverseView()
{
    if (dirtyViewMatrix)
        ConstructView();
    return inverseView;
}

Vector3 Camera::GetPosition()
{
    return position;
}

Vector3 Camera::GetRotation()
{
    return rotation;
}

float Camera::GetFOV()
{
    return fov;
}

bool Camera::IsOrthographic()
{
    return orthographic;
}

bool Camera::IsPerspective()
{
    return !orthographic;
}

void Camera::SetProj(Matrix _proj)
{
    proj = _proj;
    DirectX::BoundingFrustum::CreateFromMatrix(frustum, proj, false);
    dirtyProjMatrix = false;
}

void Camera::SetView(Matrix _view)
{
    view = _view;
    inverseView = view.Invert().Transpose();
    
    Vector3 scale, pos;
    Quaternion rot;
    view.Invert().Decompose(scale, rot, pos);
    SetPosition(pos);
    SetRotation(rot.ToEuler());

    dirtyViewMatrix = false;
}

void Camera::SetPosition(Vector3 _position)
{
    position = _position;
    dirtyViewMatrix = true;
}

void Camera::SetRotation(Vector3 _rotation)
{
    rotation = _rotation;
    dirtyViewMatrix = true;
}

void Camera::SetFOV(float _fov)
{
    fov = _fov;
    dirtyProjMatrix = true;
}

void Camera::SetOrthographic(bool _orthographic)
{
    if (orthographic != _orthographic)
        dirtyProjMatrix = true;

    orthographic = _orthographic;
}

void Camera::SetPerspective(bool _perspective)
{
    SetOrthographic(!_perspective);
}

BoundingFrustum Camera::GetFrustum()
{
    BoundingFrustum f = frustum;
    f.Origin = GetPosition();
    f.Orientation = Quaternion::CreateFromYawPitchRoll(GetRotation());
    return f;
}

Viewport Camera::GetViewport()
{
    return Viewport(viewport);
}

Vector3 Camera::WorldToScreen(Vector3 worldPosition, bool& visible)
{
    Vector3 screenPos = Viewport(viewport).Project(worldPosition, GetProj(), GetView(), Matrix::Identity);
    if (GetPosition().Dot(screenPos) > 0)
        visible = true;
    else
        visible = false;

    screenPos.x /= viewport.Width;
    screenPos.y /= viewport.Height;

    screenPos.x -= 0.5;
    screenPos.y -= 0.5;
    screenPos.x *= 2;
    screenPos.y *= 2;

    return screenPos;
}

DirectX::SimpleMath::Ray Camera::ScreenToWorldRay(int x, int y)
{
    Viewport vp = GetViewport();
    Vector3 nearPoint = vp.Unproject(Vector3((float)x, (float)y, 0.0f), GetProj(), GetView(), Matrix::Identity);
    Vector3 farPoint = vp.Unproject(Vector3((float)x, (float)y, 1.0f), GetProj(), GetView(), Matrix::Identity);
    Vector3 dir = farPoint - nearPoint;
    dir.Normalize();
    return Ray(nearPoint, dir);
}

Matrix Camera::GetBillboardMatrix(Vector3 worldPosition)
{
    Matrix billboard = Matrix::CreateBillboard(worldPosition, GetPosition(), Vector3::Down);

    return billboard;
}

void Camera::ConstructMatrices()
{
    ConstructView();
    ConstructProjection();
}

void Camera::ConstructView()
{
    view = Matrix::CreateFromYawPitchRoll(rotation) * Matrix::CreateTranslation(position);
    inverseView = view.Transpose(); // Normally it is view.Invert.Transpose() but view is already inverted so we just need to do tranpose
    view = view.Invert();
    dirtyViewMatrix = false;
}

void Camera::ConstructProjection()
{
    if (orthographic)
        proj = OrthographicProjection(orthographicSize * (viewport.Width / viewport.Height), orthographicSize, nearZ, farZ);
    else
    {
        float verticalFOV = 2 * atanf(tanf(toRad(fov) / 2) * (viewport.Height / viewport.Width));
        proj = PerspectiveFovProjection(viewport.Width, viewport.Height, toDeg(verticalFOV), nearZ, farZ);
    }

    DirectX::BoundingFrustum::CreateFromMatrix(frustum, proj, false);
    dirtyProjMatrix = false;
}


void Camera::RotateEuler(Vector3 euler, bool unlockPitch, bool unlockRoll)
{
    Vector3 rot = rotation;
    rot += euler;
    rot = EulerVectorModulo(rot);
    if (!unlockPitch)
    {
        rot.x = fmin(AchillesHalfPi - 0.005f, rot.x);
        rot.x = fmax(-AchillesHalfPi + 0.005f, rot.x);
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
