#include "Scene.h"
#include "Profiling.h"

using namespace DirectX;

Scene::Scene(std::wstring _name) : name(_name)
{
    objectTree = std::make_shared<Object>(name);
    objectTree->isScene = true;
}
Scene::~Scene()
{
    
}

void Scene::AddObjectToScene(std::shared_ptr<Object> object, std::shared_ptr<Object> parent)
{
    if (object == nullptr)
        return;
    if (parent == nullptr)
        parent = objectTree;
    parent->AddChild(object);
}

std::shared_ptr<Object> Scene::GetObjectTree()
{
    if (!hasSetObjectTreeScene)
    {
        objectTree->relatedScene = shared_from_this();
        hasSetObjectTreeScene = true;
    }
    return objectTree;
}

bool Scene::IsActive()
{
    return isActive;
}

void Scene::SetActive(bool active)
{
    isActive = active;
}

std::wstring Scene::GetName()
{
    return name;
}

void Scene::SetName(std::wstring _name)
{
    name = _name;
    objectTree->SetName(name);
}

BoundingSphere Scene::GetBoundingSphere()
{
    ScopedTimer _prof(L"GetSceneBounds");

    std::vector<std::shared_ptr<Object>> flattenedTree;
    objectTree->FlattenActive(flattenedTree);

    BoundingBox aabb;
    for (std::shared_ptr<Object> object : flattenedTree)
    {
        BoundingBox::CreateMerged(aabb, aabb, object->GetWorldAABB());
    }

    BoundingSphere bs;
    BoundingSphere::CreateFromBoundingBox(bs, aabb);

    return bs;
}