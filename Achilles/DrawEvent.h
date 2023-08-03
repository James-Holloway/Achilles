#pragma once

#include <memory>

class Object;
class Camera;

enum class DrawEventType
{
    Ignore = 0,
    DrawIndexed,
    DrawSprite,
};

struct DrawEvent
{
    std::shared_ptr<Object> object;
    std::shared_ptr<Camera> camera;
    uint32_t knitIndex;
    DrawEventType eventType;
};