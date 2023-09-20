#pragma once
#include "Achilles\Object.h"
class Planet : public Object
{
public:
    Planet();

    void SetupDrawablePlanet(std::shared_ptr<CommandList> commandList, std::wstring planetName);
    void SetupDrawableNormalMap(std::shared_ptr<CommandList> commandList);

public:
    std::shared_ptr<Object> drawable;

    float radius = 10.0f;

public:
    DirectX::BoundingSphere GetBoundingSphere();

public:
    static std::shared_ptr<Planet> CreatePlanet(std::shared_ptr<CommandList> commandList, std::wstring planetName, DirectX::SimpleMath::Vector3 position, float radius);
};

