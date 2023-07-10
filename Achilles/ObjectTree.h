#pragma once
#include "Object.h"

#ifdef GetObject
#undef GetObject
#endif

class ObjectTree;

// Return false to end traverse early
typedef bool (CALLBACK* TraverseObjectTree)(ObjectTree* objectTree);
// Called just before returning up the depth
typedef void (CALLBACK* TraverseObjectTreeUp)(ObjectTree* objectTree);

class ObjectTree
{
	friend class Scene;
public:
	// Constructors and destructor
	ObjectTree(std::wstring _emptyName = L"[No Object]");
	ObjectTree(Object* _object);
	~ObjectTree();

	// Get/set the related object
	bool IsEmpty();
	Object* GetObject();
	void SetObject(Object* _object);
	std::wstring GetName();
	void SetName(std::wstring);

	// Get/set this object's children

	ObjectTree* AddChild(Object* _object);
	ObjectTree* AddChild(ObjectTree* _object);
	bool RemoveChild(Object* _object);
	bool RemoveChild(ObjectTree* _object);
	bool RemoveChild(size_t index);
	ObjectTree*& operator[](size_t&& index);
	std::vector<ObjectTree*> GetChildren();

	// Parent functions
	ObjectTree* GetParent(); // If this is the scene object, it returns itself
	bool SetParent(ObjectTree* newParent);

	// Object Tree Traversal
	
	// Depth-first traversal of the whole object tree until traverseFunc returns false
	void Traverse(TraverseObjectTree traverseFunc, TraverseObjectTreeUp traverseUpFunc = nullptr, int maxDepth = 10, int currentDepth = 0);
	// Depth-first traversal of the whole active object tree until traverseFunc returns false
	void TraverseActive(TraverseObjectTree traverseFunc, TraverseObjectTreeUp traverseUpFunc = nullptr, int maxDepth = 10, int currentDepth = 0);

	// Depth-first search. Recursive.
	void Flatten(std::vector<ObjectTree*>& flattenedTree);
	// Depth-first search. Recursive
	void FlattenActive(std::vector<ObjectTree*>& flattenedTree);

	// Does not get empty or inactive objects. See FindAllObjectTreesBySubstring for a function that does
	std::vector<Object*> FindObjectsBySubstring(std::wstring name);
	// Also returns empty and inactive objects. See FindObjectsBySubstring for a function that doesn't
	std::vector<ObjectTree*> FindAllObjectTreesBySubstring(std::wstring name);

	// Returns the first object that matches the name, searched depth-first, including inactive objects
	Object* FindFirstObjectByName(std::wstring name);
	// Returns the first active object that matches the name, searched depth-first, excluding inactive objects
	Object* FindFirstActiveObjectByName(std::wstring name);

	// Cleanup
	void Destroy();

protected:
	Object* object = nullptr;
	std::vector<ObjectTree*> children{};
	ObjectTree* parent = nullptr;
	bool isScene = false; // Used by friend class Scene

	std::wstring emptyName = L"[No Object]";
};

