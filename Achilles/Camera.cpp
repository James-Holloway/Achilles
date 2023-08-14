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

void Camera::CreateFrustumPointsFromCascadeInterval(float intervalBegin, float intervalEnd, Vector4* frustumPoints)
{
    /*

    // Near cap
    frustumPoints[0] = { +1.0f, -1.0f, intervalBegin, +1.0f };
    frustumPoints[1] = { -1.0f, -1.0f, intervalBegin, +1.0f };
    frustumPoints[2] = { -1.0f, +1.0f, intervalBegin, +1.0f };
    frustumPoints[3] = { +1.0f, +1.0f, intervalBegin, +1.0f };
    // Far cap
    frustumPoints[4] = { +1.0f, -1.0f, intervalEnd, +1.0f };
    frustumPoints[5] = { -1.0f, -1.0f, intervalEnd, +1.0f };
    frustumPoints[6] = { -1.0f, +1.0f, intervalEnd, +1.0f };

    Matrix persinv = GetProj().Invert();

    for (int i = 0; i < 8; i++)
    {
        frustumPoints[i] = XMVector4Transform(frustumPoints[i], persinv);
    }

    //*/

    /*
    BoundingFrustum viewFrustum = frustum;
    // BoundingFrustum viewFrustum = GetFrustum();
    viewFrustum.Near = intervalBegin;
    viewFrustum.Far = intervalEnd;

    Vector3 frustumPoints3[8];
    viewFrustum.GetCorners(frustumPoints3);

    for (uint32_t i = 0; i < 8; i++)
    {
        frustumPoints[i] = Vector4(XMVECTOR(frustumPoints3[i]));
        frustumPoints[i].w = 1;
    }
    //*/

    /**/
    BoundingFrustum viewFrustum(GetProj().Transpose());
    viewFrustum.Near = intervalBegin;
    viewFrustum.Far = intervalEnd;

    static const XMVECTORU32 vGrabX = { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 };
    static const XMVECTORU32 vGrabY = { 0x00000000,0xFFFFFFFF,0x00000000,0x00000000 };

    XMVECTORF32 vRightTop = { viewFrustum.RightSlope, viewFrustum.TopSlope, 1.0f, 1.0f };
    XMVECTORF32 vLeftBottom = { viewFrustum.LeftSlope, viewFrustum.BottomSlope, 1.0f, 1.0f };
    XMVECTORF32 vNear = { viewFrustum.Near, viewFrustum.Near, viewFrustum.Near, 1.0f };
    XMVECTORF32 vFar = { viewFrustum.Far, viewFrustum.Far, viewFrustum.Far, 1.0f };

    XMVECTOR vRightTopNear = XMVectorMultiply(vRightTop, vNear);
    XMVECTOR vRightTopFar = XMVectorMultiply(vRightTop, vFar);
    XMVECTOR vLeftBottomNear = XMVectorMultiply(vLeftBottom, vNear);
    XMVECTOR vLeftBottomFar = XMVectorMultiply(vLeftBottom, vFar);

    frustumPoints[0] = vRightTopNear;
    frustumPoints[1] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabX);
    frustumPoints[2] = vLeftBottomNear;
    frustumPoints[3] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabY);

    frustumPoints[4] = vRightTopFar;
    frustumPoints[5] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabX);
    frustumPoints[6] = vLeftBottomFar;
    frustumPoints[7] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabY);
    // */

    /*
    Matrix inv = XMMatrixInverse(nullptr, GetProj() * GetView());

    uint32_t i = 0;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const Vector4 pt = XMVector4Transform(Vector4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f), inv);
                frustumPoints[i++] = pt / pt.w;
            }
        }
    }
    //*/
}