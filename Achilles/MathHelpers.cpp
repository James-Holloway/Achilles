#include "MathHelpers.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// Thank you to https://www.3dgep.com/understanding-the-view-matrix/ and https://www.youtube.com/watch?v=ih20l3pJoeU

Matrix PerspectiveFovProjection(float width, float height, float fov, float nearZ, float farZ)
{
    float aspectRatio = height / width;
    float fovAngleY = fov * AchillesPi / 180.0f;

    if (aspectRatio < 1.0f)
    {
        fovAngleY /= aspectRatio;
    }

    float cotFov = 1 / tanf(fovAngleY * 0.5f);
    float zRange = farZ - nearZ;

    Matrix perspective;
    perspective.m[0][0] = aspectRatio * cotFov;
    perspective.m[0][1] = 0;
    perspective.m[0][2] = 0;
    perspective.m[0][3] = 0;

    perspective.m[1][0] = 0;
    perspective.m[1][1] = cotFov;
    perspective.m[1][2] = 0;
    perspective.m[1][3] = 0;

    perspective.m[2][0] = 0;
    perspective.m[2][1] = 0;
    perspective.m[2][2] = farZ / zRange;
    perspective.m[2][3] = 1;

    perspective.m[3][0] = 0;
    perspective.m[3][1] = 0;
    perspective.m[3][2] = (-farZ * nearZ) / zRange;
    perspective.m[3][3] = 0;

    return perspective;
}

Matrix OrthographicProjection(float offsetX, float width, float offsetY, float height, float nearZ, float farZ)
{
    // Map nearZ/farZ to 0-1
    float f = farZ;// 1.0f;
    float n = nearZ; //nearZ / farZ;
    Matrix orthographic = DirectX::XMMatrixOrthographicOffCenterLH(offsetX, offsetX + width, offsetY, offsetY + height, n, f);
    // orthographic._22 = -orthographic._22;
    return orthographic.Transpose();
    /*
    float left = offsetX;
    float right = left + width;
    float top = offsetY;
    float bottom = top + height;

    // Map nearZ/farZ to 0-1
    float f = 1;
    float n = nearZ / farZ;

    Matrix orthographic;
    */
    /*
    orthographic.m[0][0] = 2.0f / (right - left); // x *= 2 / width
    orthographic.m[0][1] = 0;
    orthographic.m[0][2] = 0;
    orthographic.m[0][3] = 0;

    orthographic.m[1][0] = 0;
    orthographic.m[1][1] = 2.0f / (bottom - top); // y *= 2 / height
    orthographic.m[1][2] = 0;
    orthographic.m[1][3] = 0;

    orthographic.m[2][0] = 0;
    orthographic.m[2][1] = 0;
    orthographic.m[2][2] = -2.0f / (f - n); // z *= 2 / (far - near),
    orthographic.m[2][3] = 0;

    orthographic.m[3][0] = -((right + left) / (right - left));
    orthographic.m[3][1] = ((top + bottom)) / (bottom - top);
    orthographic.m[3][2] = ((f + n) / (f - n));
    orthographic.m[3][3] = 1.0f;
    */
    /*
    orthographic.m[0][0] = 2.0f / (right - left); // x *= 2 / width
    orthographic.m[0][1] = 0;
    orthographic.m[0][2] = 0;
    orthographic.m[0][3] = 0;

    orthographic.m[1][0] = 0;
    orthographic.m[1][1] = 2.0f / (top - bottom); // y *= 2 / height
    orthographic.m[1][2] = 0;
    orthographic.m[1][3] = 0;

    orthographic.m[2][0] = 0;
    orthographic.m[2][1] = 0;
    orthographic.m[2][2] = 1.0f / (f - n); // z *= 1 / (far - near),
    orthographic.m[2][3] = 0;

    orthographic.m[3][0] = -1;
    orthographic.m[3][1] = 1;
    orthographic.m[3][2] = -((f + n) / (f - n));
    //orthographic.m[3][2] = -(n / (n - f));
    orthographic.m[3][3] = 1.0f;

    return orthographic;
    */
}

Matrix OrthographicProjection(float width, float height, float nearZ, float farZ)
{
    return OrthographicProjection(-width, width * 2, -height, height * 2, nearZ, farZ);
}

// https://stackoverflow.com/a/1171995
Quaternion DirectionToQuaternion(Vector3 pos, Vector3 direction)
{
    Vector3 a = pos + direction;

    float dot = pos.Dot(a);
    if (dot > 0.99999) // paralllel vectors
    {
        return Quaternion::Identity;
    }
    else if (dot < -0.99999) // opposite vectors
    {
        return Quaternion::CreateFromYawPitchRoll(0, AchillesPi, 0);
    }

    Quaternion q{};
    Vector3 b = pos.Cross(a);

    q.x = b.x;
    q.y = b.y;
    q.z = b.z;
    q.w = sqrtf(pos.LengthSquared() * a.LengthSquared()) + dot;

    return q;
}

Matrix ViewLookAt(Vector3 eye, Vector3 target, Vector3 up)
{
    Vector3 z = (eye - target);
    z.Normalize();
    Vector3 x = up.Cross(z);
    x.Normalize();
    Vector3 y = z.Cross(x);

    Matrix viewMatrix = {
        x.x, y.x, z.x, 0,
        x.y, y.y, z.y, 0,
        x.z, y.z, z.z, 0,
        -x.Dot(eye), -y.Dot(eye), -z.Dot(eye), 1
    };

    return viewMatrix;
}

Quaternion ViewLookAt(Vector3 forward, Vector3 up)
{
    // Given, but ensure normalization
    float forwardLenSq = forward.LengthSquared();
    forward = forwardLenSq > 0.000001f ? forward * RecipSqrt(Vector3(forwardLenSq)) : -Vector3::UnitZ;

    // Deduce a valid, orthogonal right vector
    Vector3 right = forward.Cross(up);
    float rightLenSq = right.LengthSquared();
    right = rightLenSq > 0.000001f ? right * RecipSqrt(Vector3(rightLenSq)) : (Vector3)(Quaternion(Vector3::UnitZ, -DirectX::XM_PIDIV2) * forward);

    // Compute actual up vector
    up = right.Cross(forward);

    // Finish constructing basis
    Matrix basis = Matrix(right, up, -forward);
    return Quaternion::CreateFromRotationMatrix(basis);
}

Vector3 Multiply(Quaternion rotation, Vector3 point)
{
    float a = rotation.x * 2.0f;
    float b = rotation.y * 2.0f;
    float c = rotation.z * 2.0f;
    float d = rotation.x * a;
    float e = rotation.y * b;
    float f = rotation.z * c;
    float g = rotation.x * b;
    float h = rotation.x * c;
    float i = rotation.y * c;
    float j = rotation.w * a;
    float k = rotation.w * b;
    float l = rotation.w * c;
    Vector3 vector;
    vector.x = (1.0f - (e + f)) * point.x + (g - l) * point.y + (h + k) * point.z;
    vector.y = (g + l) * point.x + (1.0f - (d + f)) * point.y + (i - j) * point.z;
    vector.z = (h - k) * point.x + (i + j) * point.y + (1.0f - (d + e)) * point.z;
    return vector;
}

// https://github.com/PanosK92/SpartanEngine/blob/2c14b0f6a00791064661a2ac966d7e07792fc955/runtime/Math/Matrix.h#L391
Vector3 Multiply(Matrix matrix, Vector3 rhs)
{
    Vector4 vWorking;

    vWorking.x = (rhs.x * matrix._11) + (rhs.y * matrix._21) + (rhs.z * matrix._31) + matrix._41;
    vWorking.y = (rhs.x * matrix._12) + (rhs.y * matrix._22) + (rhs.z * matrix._32) + matrix._42;
    vWorking.z = (rhs.x * matrix._13) + (rhs.y * matrix._23) + (rhs.z * matrix._33) + matrix._43;
    vWorking.w = 1 / ((rhs.x * matrix._14) + (rhs.y * matrix._24) + (rhs.z * matrix._34) + matrix._44);

    return Vector3(vWorking.x * vWorking.w, vWorking.y * vWorking.w, vWorking.z * vWorking.w);
}

Matrix DirectionToRotationMatrix(Vector3 dir, Vector3 up)
{
    Vector3 xAxis = up.Cross(dir);
    xAxis.Normalize();
    Vector3 yAxis = dir.Cross(xAxis);
    yAxis.Normalize();

    Matrix matrix = Matrix::Identity;
    matrix._11 = xAxis.x;
    matrix._12 = yAxis.x;
    matrix._13 = dir.x;

    matrix._21 = xAxis.y;
    matrix._22 = yAxis.y;
    matrix._23 = dir.y;

    matrix._31 = xAxis.z;
    matrix._32 = yAxis.z;
    matrix._33 = dir.z;

    return matrix;
}
