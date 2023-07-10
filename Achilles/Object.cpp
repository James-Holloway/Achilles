#include "Object.h"
#include "Mesh.h"


//// Private constructor & destructor functions ////

Object::Object(std::wstring _name) : name(_name), mesh(nullptr)
{
}

Object::Object(std::shared_ptr<Mesh> _mesh, std::wstring _name) : name(_name), mesh(_mesh)
{

}

Object::~Object()
{
	
}


//// Name functions ////

std::wstring Object::GetName()
{
	if (isScene)
		return L"[Scene]";
	return name;
}

void Object::SetName(std::wstring _name)
{
	if (isScene)
		return;
	name = _name;
}


//// Mesh functions ////

std::shared_ptr<Mesh> Object::GetMesh()
{
	return mesh;
}

void Object::SetMesh(std::shared_ptr<Mesh> _mesh)
{
	mesh = _mesh;
}


//// Empty / Active functions ////

bool Object::IsEmpty()
{
	return mesh == nullptr;
}

bool Object::IsActive()
{
	return active;
}

void Object::SetActive(bool _active)
{
	active = _active;
}


//// Get/set this object's children ////

std::shared_ptr<Object> Object::MoveChild(std::shared_ptr<Object> _object, std::shared_ptr<Object> newParent)
{
	return newParent->AddChild(_object);
}

std::shared_ptr<Object> Object::AddChild(std::shared_ptr<Object> _object)
{
	if (_object == shared_from_this())
		throw std::exception("An Object cannot be a child of itself");

	children.push_back(_object);
	_object->SetParent(shared_from_this());
	return _object;
}

bool Object::RemoveChild(std::shared_ptr<Object> _object)
{
	size_t erased = std::erase(children, _object);
	return erased > 0;
}

bool Object::RemoveChild(size_t index)
{
	if (index < 0 || index >= children.size())
		return false;
	children.erase(children.begin() + index);
	return true;
}

std::shared_ptr<Object> & Object::operator[](size_t&& index)
{
	return children[index];
}

std::vector<std::shared_ptr<Object>> Object::GetChildren()
{
	return children;
}


//// Parent functions ////

std::shared_ptr<Object> Object::GetParent()
{
	if (isScene)
		return shared_from_this();
	return parent;
}

bool Object::SetParent(std::shared_ptr<Object> newParent)
{
	if (isScene)
		return false;
	if (newParent == shared_from_this())
		throw std::exception("An Object cannot be a parent of itself");
	if (parent != nullptr)
	{
		parent->RemoveChild(shared_from_this());
	}
	parent = newParent;
	return true;
}


//// Object tree traversal ////

void Object::Traverse(TraverseObject traverseFunc, TraverseObjectUp traverseUpFunc, int maxDepth, int currentDepth)
{
	if (traverseFunc == nullptr)
		return;

	if (!isScene)
	{
		bool result = traverseFunc(shared_from_this());
		if (!result)
			return;
	}

	if (currentDepth >= maxDepth)
		return;

	for (std::shared_ptr<Object> child : children)
	{
		child->Traverse(traverseFunc, traverseUpFunc, maxDepth, currentDepth + 1);
	}

	if (!isScene && traverseUpFunc)
	{
		traverseUpFunc(shared_from_this());
	}
}

void Object::TraverseActive(TraverseObject traverseFunc, TraverseObjectUp traverseUpFunc, int maxDepth, int currentDepth)
{
	if (!isScene && IsActive())
	{
		bool result = traverseFunc(shared_from_this());
		if (!result)
			return;
	}

	if (currentDepth >= maxDepth)
		return;

	for (std::shared_ptr<Object> child : children)
	{
		child->TraverseActive(traverseFunc, traverseUpFunc, maxDepth, currentDepth + 1);
	}
	if (!isScene && traverseUpFunc)
	{
		traverseUpFunc(shared_from_this());
	}
}

void Object::Flatten(std::vector<std::shared_ptr<Object>>& flattenedTree)
{
	flattenedTree.push_back(shared_from_this());
	for (std::shared_ptr<Object> child : children)
	{
		child->Flatten(flattenedTree);
	}
}

void Object::FlattenActive(std::vector<std::shared_ptr<Object>>& flattenedTree)
{
	if (IsActive() && !isScene)
	{
		flattenedTree.push_back(shared_from_this());
	}
	for (std::shared_ptr<Object> child : children)
	{
		child->Flatten(flattenedTree);
	}
}

std::vector<std::shared_ptr<Object>> Object::FindObjectsBySubstring(std::wstring name)
{
	std::vector<std::shared_ptr<Object>> objects;
	FlattenActive(objects);

	std::vector<std::shared_ptr<Object>> matchedObjects;
	for (std::shared_ptr<Object> object : objects)
	{
		if (object == nullptr || object->IsEmpty())
			continue;
		std::wstring objectName = object->GetName();
		if (Contains(objectName, name))
			matchedObjects.push_back(object);
	}

	return matchedObjects;
}

std::vector<std::shared_ptr<Object>> Object::FindAllObjectsBySubstring(std::wstring name)
{
	std::vector<std::shared_ptr<Object>> objects;
	Flatten(objects);

	std::vector<std::shared_ptr<Object>> matchedObjects;
	for (std::shared_ptr<Object> object : objects)
	{
		if (object == nullptr || object->IsEmpty())
			continue;
		std::wstring objectName = object->GetName();
		if (Contains(objectName, name))
			matchedObjects.push_back(object);
	}
	return matchedObjects;
}

std::shared_ptr<Object> Object::FindFirstObjectByName(std::wstring name)
{
	std::vector<std::shared_ptr<Object>> objects;
	Flatten(objects);

	for (std::shared_ptr<Object> object : objects)
	{
		if (object == nullptr || object->IsEmpty())
			continue;

		if (name == object->GetName())
			return object;
	}
	return nullptr;
}

std::shared_ptr<Object> Object::FindFirstActiveObjectByName(std::wstring name)
{
	std::vector<std::shared_ptr<Object>> objects;
	FlattenActive(objects);

	for (std::shared_ptr<Object> object : objects)
	{
		if (object == nullptr || object->IsEmpty())
			continue;

		if (name == object->GetName())
			return object;
	}
	return nullptr;
}


//// Static Object creation functions ////

std::shared_ptr<Object> Object::CreateObject(std::wstring name, std::shared_ptr<Object> parent)
{
	return CreateObject(nullptr, name, parent);
}

std::shared_ptr<Object> Object::CreateObject(std::shared_ptr<Mesh> mesh, std::wstring name, std::shared_ptr<Object> parent)
{
	std::shared_ptr<Object> object = std::make_shared<Object>(mesh, name);
	object->SetParent(parent);

	return object;
}
