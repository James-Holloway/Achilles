#pragma once
#include "Common.h"

class Mesh;
class ObjectTree;

class Object
{
	friend class ObjectTree;
public:
	Object(std::wstring _name= L"Unnamed Object");
	Object(Mesh* _mesh, std::wstring _name = L"Unnamed Object");
	~Object();

	ObjectTree* GetObjectTree();
	ObjectTree* GetParent();
	std::vector<ObjectTree*> GetChildren();

	std::wstring GetName();
	void SetName(std::wstring _name);

	Mesh* GetMesh();
	void SetMesh(Mesh* _mesh);

	bool IsActive();
	void SetActive(bool _active);

protected:
	// Friend ObjectTree functions
	void SetObjectTree(ObjectTree* _objectTree);

protected:
	std::wstring name;
	Mesh* mesh;
	ObjectTree* objectTree;
	bool active = true;
};
