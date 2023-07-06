#pragma once
#include "Common.h"
#include "Mesh.h"
#include "Camera.h"

enum class DrawEventType
{
	Ignore = 0,
	DrawIndexed
};

struct DrawEvent
{
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Camera> camera;
	DrawEventType eventType;
};