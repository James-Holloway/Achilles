#pragma once
#include "Common.h"
#include "ObjectTree.h"

class Scene
{
public:
	Scene(std::wstring _name = L"Scene");
	~Scene();

	// By adding an ObjectTree to a scene, it will then be deleted when its parent is deleted
	// @param objectTree
	// @param parent is the scene's ObjectTree when nullptr
	void AddObjectToScene(ObjectTree* objectTree, ObjectTree* parent = nullptr);
	// By adding an Object to a scene, it will then be deleted when the its parent is deleted
	// @param parent is the scene's ObjectTree when nullptr
	void AddObjectToScene(Object* object, ObjectTree* parent = nullptr);

	ObjectTree* GetObjectTree();

	bool IsActive();
	void SetActive(bool active);

	std::wstring GetName();
	void SetName(std::wstring _name);

protected:
	bool isActive = false;
	ObjectTree* objectTree;
	std::wstring name;
};
