#pragma once
#include <memory>
#include "Material.h"

class Mesh;

// A knit is a mesh and material combo
struct Knit
{
    std::shared_ptr<Mesh> mesh;
    Material material;
    Knit() : material{}, mesh(nullptr)
    {

    }

    Knit(const Knit& other)
    {
        mesh = other.mesh;
        material = Material(other.material);
    }
};