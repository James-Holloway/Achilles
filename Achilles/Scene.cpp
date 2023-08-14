#include "Scene.h"
#include "Profiling.h"

using namespace DirectX;

static uint32_t globalSceneIndex = 0;
Scene::Scene(std::wstring _name) : name(_name)
{
    objectTree = std::make_shared<Object>(name);
    objectTree->isScene = true;
    sceneIndex = globalSceneIndex++;
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

DirectX::BoundingBox Scene::GetBoundingBox()
{
    ScopedTimer _prof(L"Get Scene Bounding Box");

    std::vector<std::shared_ptr<Object>> flattenedTree;
    objectTree->FlattenActive(flattenedTree);

    BoundingBox aabb;
    for (std::shared_ptr<Object> object : flattenedTree)
    {
        BoundingBox::CreateMerged(aabb, aabb, object->GetWorldAABB());
    }
    return aabb;
}

BoundingSphere Scene::GetBoundingSphere()
{
    BoundingBox aabb = GetBoundingBox();

    BoundingSphere bs;
    BoundingSphere::CreateFromBoundingBox(bs, aabb);

    return bs;
}

uint32_t Scene::GetSceneIndex()
{
    return sceneIndex;
}
