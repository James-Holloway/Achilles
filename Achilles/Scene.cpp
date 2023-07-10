#include "Scene.h"

Scene::Scene(std::wstring _name) : name(_name)
{
	objectTree = new ObjectTree();
	objectTree->isScene = true;
}
Scene::~Scene()
{
	SAFE_DELETE(objectTree);
}

void Scene::AddObjectToScene(ObjectTree* object, ObjectTree* parent)
{
	if (parent == nullptr)
		parent = object;
	parent->AddChild(object);
}
void Scene::AddObjectToScene(Object* object, ObjectTree* parent)
{
	if (parent == nullptr)
		parent = objectTree;
	parent->AddChild(object);
}

ObjectTree* Scene::GetObjectTree()
{
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
}
