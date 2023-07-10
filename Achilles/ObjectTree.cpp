#include "ObjectTree.h"

// Constructors and destructor
ObjectTree::ObjectTree(std::wstring _emptyName) : emptyName(_emptyName)
{

}

ObjectTree::ObjectTree(Object* _object) : object(_object)
{
	if (object != nullptr)
		object->SetObjectTree(this);
}

ObjectTree::~ObjectTree()
{
	Destroy();
}

bool ObjectTree::IsEmpty()
{
	return object == nullptr;
}

// Get/set the related object
Object* ObjectTree::GetObject()
{
	return object;
}

void ObjectTree::SetObject(Object* _object)
{
	object = _object;
	if (object != nullptr)
		object->SetObjectTree(this);
}

std::wstring ObjectTree::GetName()
{
	if (isScene)
		return L"[Scene]";
	if (object == nullptr)
		return emptyName;
	return object->GetName();
}

void ObjectTree::SetName(std::wstring name)
{
	if (isScene)
		return;
	if (object != nullptr)
		object->SetName(name);
	emptyName = name + L" (empty)"; // set the empty name just in case SetObject(nullptr)
}

// Get/set this object's children
ObjectTree* ObjectTree::AddChild(Object* _object)
{
	return AddChild(new ObjectTree(_object));
}

ObjectTree* ObjectTree::AddChild(ObjectTree* _object)
{
	if (_object == this)
		throw std::exception("An ObjectTree cannot be a child of itself");

	children.push_back(_object);
	_object->SetParent(this);
	return _object;
}

bool ObjectTree::RemoveChild(Object* _object)
{
	return RemoveChild(_object->GetObjectTree());
}

bool ObjectTree::RemoveChild(ObjectTree* _object)
{
	size_t erased = std::erase(children, _object);
	return erased > 0;
}

bool ObjectTree::RemoveChild(size_t index)
{
	if (index < 0 || index >= children.size())
		return false;
	children.erase(children.begin() + index);
	return true;
}

ObjectTree*& ObjectTree::operator[](size_t&& index)
{
	return children[index];
}

std::vector<ObjectTree*> ObjectTree::GetChildren()
{
	return children;
}

// Parent functions
ObjectTree* ObjectTree::GetParent()
{
	if (isScene)
		return this;
	return parent;
}

bool ObjectTree::SetParent(ObjectTree* newParent)
{
	if (isScene)
		return false;
	if (newParent == this)
		throw std::exception("An ObjectTree cannot be a parent of itself");
	if (parent != nullptr)
	{
		parent->RemoveChild(this);
	}
	parent = newParent;
	return true;
}

// Object Tree Traversal
void ObjectTree::Traverse(TraverseObjectTree traverseFunc, TraverseObjectTreeUp traverseUpFunc, int maxDepth, int currentDepth)
{
	if (traverseFunc == nullptr)
		return;

	if (!isScene)
	{
		bool result = traverseFunc(this);
		if (!result)
			return;
	}

	if (currentDepth >= maxDepth)
		return;

	for (ObjectTree* child : children)
	{
		child->Traverse(traverseFunc, traverseUpFunc, maxDepth, currentDepth + 1);
	}
	
	if (!isScene && traverseUpFunc)
	{
		traverseUpFunc(this);
	}
}

void ObjectTree::TraverseActive(TraverseObjectTree traverseFunc, TraverseObjectTreeUp traverseUpFunc, int maxDepth, int currentDepth)
{
	if (!isScene && this->GetObject() && this->GetObject()->IsActive())
	{
		bool result = traverseFunc(this);
		if (!result)
			return;
	}

	if (currentDepth >= maxDepth)
		return;

	for (ObjectTree* child : children)
	{
		child->TraverseActive(traverseFunc, traverseUpFunc, maxDepth, currentDepth + 1);
	}
	if (!isScene && traverseUpFunc)
	{
		traverseUpFunc(this);
	}
}

void ObjectTree::Flatten(std::vector<ObjectTree*>& flattenedTree)
{
	flattenedTree.push_back(this);
	for (ObjectTree* child : children)
	{
		child->Flatten(flattenedTree);
	}
}

void ObjectTree::FlattenActive(std::vector<ObjectTree*>& flattenedTree)
{
	if (object != nullptr && object->IsActive() && !isScene)
	{
		flattenedTree.push_back(this);
	}
	for (ObjectTree* child : children)
	{
		child->Flatten(flattenedTree);
	}
}

std::vector<Object*> ObjectTree::FindObjectsBySubstring(std::wstring name)
{
	std::vector<ObjectTree*> objects;
	FlattenActive(objects);

	std::vector<Object*> matchedObjects;
	for (ObjectTree* objectTree : objects)
	{
		if (objectTree == nullptr || objectTree->IsEmpty())
			continue;
		std::wstring objectName = objectTree->GetName();
		if (Contains(objectName, name))
			matchedObjects.push_back(objectTree->GetObject());
	}

	return matchedObjects;
}

std::vector<ObjectTree*> ObjectTree::FindAllObjectTreesBySubstring(std::wstring name)
{
	std::vector<ObjectTree*> objects;
	Flatten(objects);

	std::vector<ObjectTree*> matchedObjects;
	for (ObjectTree* objectTree : objects)
	{
		if (objectTree == nullptr || objectTree->IsEmpty())
			continue;
		std::wstring objectName = objectTree->GetName();
		if (Contains(objectName, name))
			matchedObjects.push_back(objectTree);
	}
	return matchedObjects;
}

Object* ObjectTree::FindFirstObjectByName(std::wstring name)
{
	std::vector<ObjectTree*> objects;
	Flatten(objects);

	for (ObjectTree* objectTree : objects)
	{
		if (objectTree == nullptr || objectTree->IsEmpty())
			continue;

		if (name == objectTree->GetName())
			return objectTree->GetObject();
	}
	return nullptr;
}

Object* ObjectTree::FindFirstActiveObjectByName(std::wstring name)
{
	std::vector<ObjectTree*> objects;
	FlattenActive(objects);

	for (ObjectTree* objectTree : objects)
	{
		if (objectTree == nullptr || objectTree->IsEmpty())
			continue;

		if (name == objectTree->GetName())
			return objectTree->GetObject();
	}
	return nullptr;
}

// Cleanup
void ObjectTree::Destroy()
{
	SAFE_DELETE(object);
	for (ObjectTree* child : children)
	{
		SAFE_DELETE(child);
	}
}
