#pragma once
#include <DirectXMath.h>
#include <directxtk12/SimpleMath.h>

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Quaternion;

constexpr float AchillesPi = 3.1415926535f;
constexpr float Achilles2Pi = 6.2831853070f;
constexpr float AchillesHalfPi = 1.5707963268f;
constexpr float AchillesQuaterPi = 0.7853981634f;

// Thank you to https://www.3dgep.com/understanding-the-view-matrix/ and https://www.youtube.com/watch?v=ih20l3pJoeU

inline Matrix PerspectiveFovProjection(float width, float height, float fov, float nearZ, float farZ)
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

inline Matrix ViewLookAt(Vector3 eye, Vector3 target, Vector3 up)
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