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

inline std::wstring ObjectTagToWString(ObjectTag tag)
{
    switch (tag)
    {
    case ObjectTag::None:
        return L"None";
    case ObjectTag::Mesh:
        return L"Mesh";
    case ObjectTag::Sprite:
        return L"Sprite";
    case ObjectTag::Light:
    case ObjectTag::Sprite | ObjectTag::Light:
        return L"Light";
    case ObjectTag::Camera:
        return L"Camera";
    }
    return L"Unknown (" + std::to_wstring((size_t)tag) + L")";
}