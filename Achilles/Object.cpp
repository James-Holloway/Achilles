#include "Object.h"
#include "ObjectTree.h"
#include "Mesh.h"

Object::Object(std::wstring _name) : name(_name), mesh(nullptr)
{
}

Object::Object(Mesh* _mesh, std::wstring _name) : name(_name), mesh(_mesh)
{

}

Object::~Object()
{
	SAFE_DELETE(mesh);
}

ObjectTree* Object::GetObjectTree()
{
	return objectTree;
}

ObjectTree* Object::GetParent()
{
	if (objectTree)
		return objectTree->GetParent();
	return nullptr;
}

std::vector<ObjectTree*> Object::GetChildren()
{
	if (objectTree)
		return objectTree->GetChildren();
	return std::vector<ObjectTree*>();
}

std::wstring Object::GetName()
{
	return name;
}

void Object::SetName(std::wstring _name)
{
	name = _name;
}

Mesh* Object::GetMesh()
{
	return mesh;
}

void Object::SetMesh(Mesh* _mesh)
{
	mesh = _mesh;
}

bool Object::IsActive()
{
	return active;
}

void Object::SetActive(bool _active)
{
	active = _active;
}

void Object::SetObjectTree(ObjectTree* _objectTree)
{
	objectTree = _objectTree;
}
