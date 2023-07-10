#include "Scene.h"

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
