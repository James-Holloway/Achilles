#include "Planet.h"
#include "Achilles/shaders/BlinnPhong.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Planet::Planet()
{

}

void Planet::SetupDrawablePlanet(std::shared_ptr<CommandList> commandList, std::wstring planetName)
{
    std::shared_ptr<Texture> planetTexture = std::make_shared<Texture>();
    commandList->LoadTextureFromContent(*planetTexture, planetName + L" Planet Diffuse");

    drawable = Object::CreateObjectsFromContentFile(L"uv sphere high.fbx", BlinnPhong::GetBlinnPhongShader(nullptr));
    drawable->GetMaterial().SetTexture(L"MainTexture", planetTexture);
    Texture::AddCachedTexture(planetTexture->GetName(), planetTexture);
    AddChild(drawable);

    SetName(planetName);
}

void Planet::SetupDrawableNormalMap(std::shared_ptr<CommandList> commandList)
{
    if (drawable == nullptr)
    {
        throw std::exception("Planet's normal map attempted to be loaded before drawable was created");
        return;
    }

    std::shared_ptr<Texture> normalTexture = std::make_shared<Texture>();
    commandList->LoadTextureFromContent(*normalTexture, GetName() + L" Planet Normal");
    Texture::AddCachedTexture(normalTexture->GetName(), normalTexture);
    drawable->GetMaterial().SetTexture(L"NormalTexture", normalTexture);
}

BoundingSphere Planet::GetBoundingSphere()
{
    return BoundingSphere(GetWorldPosition(), radius);
}

std::shared_ptr<Planet> Planet::CreatePlanet(std::shared_ptr<CommandList> commandList, std::wstring planetName, Vector3 position, float radius)
{
    std::shared_ptr<Planet> planet = std::make_shared<Planet>();
    planet->SetupDrawablePlanet(commandList, planetName);
    planet->SetWorldPosition(position);
    planet->radius = radius;
    planet->SetWorldScale(Vector3(radius));
    return planet;
}
