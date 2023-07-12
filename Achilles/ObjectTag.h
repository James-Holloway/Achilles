#pragma once

#include <winnt.h>

enum class ObjectTag
{
    None = 0,
    Mesh = 1,
    Sprite = 2,
    Light = 4,
    Camera = 8,
};
DEFINE_ENUM_FLAG_OPERATORS(ObjectTag);