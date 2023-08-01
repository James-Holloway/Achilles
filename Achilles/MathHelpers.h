#pragma once
#include <DirectXMath.h>
#include <directxtk12/SimpleMath.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Quaternion;

constexpr float AchillesPi = 3.1415926535f;
constexpr float Achilles2Pi = 6.2831853070f;
constexpr float AchillesHalfPi = 1.5707963268f;
constexpr float AchillesQuaterPi = 0.7853981634f;

inline Vector3 Recip(Vector3 vec)
{
    return XMVectorReciprocal(vec);
}
inline Vector3 RecipSqrt(Vector3 vec)
{
    return DirectX::XMVectorReciprocalSqrt(vec);
}

inline Vector4 Recip(Vector4 vec)
{
    return XMVectorReciprocal(vec);
}
inline Vector4 RecipSqrt(Vector4 vec)
{
    return DirectX::XMVectorReciprocalSqrt(vec);
}

Matrix PerspectiveFovProjection(float width, float height, float fov, float nearZ, float farZ);

Matrix OrthographicProjection(float offsetX, float width, float offsetY, float height, float nearZ, float farZ);

Matrix OrthographicProjection(float width, float height, float nearZ, float farZ);

Matrix ViewLookAt(Vector3 eye, Vector3 target, Vector3 up);

Quaternion ViewLookAt(Vector3 forward, Vector3 up);

inline Matrix ViewFPS(Vector3 eye, float pitch, float yaw)
{
    float cosPitch = cosf(pitch);
    float sinPitch = sinf(pitch);
    float cosYaw = sinf(pitch);
    float sinYaw = sinf(yaw);

    Vector3 x{ cosYaw, 0, -sinYaw };
    Vector3 y{ sinYaw * sinPitch, cosPitch, cosYaw * sinPitch };
    Vector3 z{ sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw };

    Matrix viewMatrix = {
        x.x, y.x, z.x, 0,
        x.y, y.y, z.y, 0,
        x.z, y.z, z.z, 0,
        -x.Dot(eye), -y.Dot(eye), -z.Dot(eye), 1
    };

    return viewMatrix;
}

inline Matrix ViewFPS(Vector3 eye, Quaternion rotation)
{
    Quaternion q;
    rotation.Normalize(q);
    Vector3 r = q.ToEuler();
    float pitch = r.y;
    float yaw = r.x;
    return ViewFPS(eye, pitch, yaw);
}


constexpr Vector3 eulerMaxVector = Vector3(Achilles2Pi, Achilles2Pi, Achilles2Pi);
inline Vector3 EulerVectorModulo(Vector3 euler)
{
    Vector3 newEuler = {
        fmod(euler.x, Achilles2Pi),
        fmod(euler.y, Achilles2Pi),
        fmod(euler.z, Achilles2Pi),
    };
    return newEuler;
}

inline float toDeg(float rad)
{
    return DirectX::XMConvertToDegrees(rad);
}

inline float toRad(float deg)
{
    return DirectX::XMConvertToRadians(deg);
}

inline Vector3 EulerToDegrees(Vector3 radians)
{
    Vector3 degrees = {
        toDeg(radians.x),
        toDeg(radians.y),
        toDeg(radians.z)
    };
    return degrees;
}

inline Vector3 EulerToRadians(Vector3 degrees)
{
    Vector3 radians = {
        toRad(degrees.x),
        toRad(degrees.y),
        toRad(degrees.z)
    };
    return radians;
}


template <typename T>
inline T DivideByMultiple(T value, size_t alignment)
{
    return (T)((value + alignment - 1) / alignment);
}

template <typename T>
inline T AlignUpWithMask(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T>
inline T AlignDownWithMask(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

template <typename T>
inline T AlignUp(T value, size_t alignment)
{
    return AlignUpWithMask(value, alignment - 1);
}

inline uint32_t NextHighestPow2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

inline uint64_t NextHighestPow2(uint64_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;

    return v;
}

// Normalize a value in the range [min - max]
template<typename T, typename U>
inline T NormalizeRange(U x, U min, U max)
{
    return T(x - min) / T(max - min);
}

template<typename T>
inline T Deadzone(T val, T deadzone)
{
    if (std::abs(val) < deadzone)
    {
        return T(0);
    }

    return val;
}

inline Vector3 QuaternionToAxisAngle(Quaternion quat, float& angle)
{
    // Normalize quaternion
    if (quat.w > 1)
        quat.Normalize();

    angle = 2 * acosf(quat.w);
    float s = sqrtf(1 - quat.w * quat.w);
    if (s < 0.001) // test to avoid divide by zero
    {
        return Vector3(quat.x, quat.y, quat.z);
    }
    return Vector3(quat.x, quat.y, quat.z) / s;
}

inline Quaternion AxisAngleToQuaternion(Vector3 axis, float angle)
{
    float s = sinf(angle / 2);
    return Quaternion(axis.x * s, axis.y * s, axis.z * s, cosf(angle / 2));
}

Quaternion DirectionToQuaternion(Vector3 pos, Vector3 direction);

Vector3 Multiply(Quaternion rotation, Vector3 point);

Vector3 Multiply(Matrix matrix, Vector3 rhs);

inline Quaternion Inverse(Quaternion quat)
{
    const float length_squared = quat.LengthSquared();
    if (length_squared == 1.0f)
    {
        quat.Conjugate();
        return quat;
    }
    else if (length_squared >= std::numeric_limits<float>::epsilon())
    {
        quat.Conjugate();
        return quat * (1.0f / length_squared);
    }
    else
    {
        return Quaternion::Identity;
    }
}

Matrix DirectionToRotationMatrix(Vector3 dir, Vector3 up = Vector3::Up);

inline uint32_t ColorPack(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
{
    uint32_t out;
    out = r << 0;
    out |= g << 8;
    out |= b << 16;
    out |= a << 24;
    return out;
}