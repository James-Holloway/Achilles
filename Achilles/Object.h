#pragma once
#include "Common.h"

class Mesh;
class Object;

// Return false to end traverse early
typedef bool (CALLBACK* TraverseObject)(std::shared_ptr<Object> object);
// Called just before returning up the depth
typedef void (CALLBACK* TraverseObjectUp)(std::shared_ptr<Object> object);

class Object : public std::enable_shared_from_this<Object>
{
	friend class Scene; // Scene can set isScene and instantiate an empty constructor
public:
	//// Public constructors & destructor functions ////

	// Should not be used. Use CreateObject instead!
	Object(std::wstring _name= L"Unnamed Object");
	// Should not be used. Used CreateObject instead!
	Object(std::shared_ptr<Mesh> _mesh, std::wstring _name = L"Unnamed Object");
	virtual ~Object();

public:
	//// Name functions ////

	std::wstring GetName();
	void SetName(std::wstring _name);


	//// Mesh functions ////

	std::shared_ptr<Mesh> GetMesh();
	void SetMesh(std::shared_ptr<Mesh> _mesh);


	//// Empty / Active functions ////

	bool IsEmpty();
	bool IsActive();
	void SetActive(bool _active);


	////  Get/set this object's children ////

	// Basically an alias for AddChild
	static std::shared_ptr<Object> MoveChild(std::shared_ptr<Object> _object, std::shared_ptr<Object> newParent);
	std::shared_ptr<Object> AddChild(std::shared_ptr<Object> _object);
	bool RemoveChild(std::shared_ptr<Object> _object);
	bool RemoveChild(size_t index);
	std::shared_ptr<Object>& operator[](size_t&& index);
	std::vector<std::shared_ptr<Object>> GetChildren();


	//// Parent functions ////

	// If this is the scene object, it returns itself
	std::shared_ptr<Object> GetParent();
	bool SetParent(std::shared_ptr<Object> newParent);


	//// Object tree traversal ////

	// Depth-first traversal of the whole object tree until traverseFunc returns false
	void Traverse(TraverseObject traverseFunc, TraverseObjectUp traverseUpFunc = nullptr, int maxDepth = 10, int currentDepth = 0);
	// Depth-first traversal of the whole active object tree until traverseFunc returns false
	void TraverseActive(TraverseObject traverseFunc, TraverseObjectUp traverseUpFunc = nullptr, int maxDepth = 10, int currentDepth = 0);

	// Depth-first search. Recursive.
	void Flatten(std::vector<std::shared_ptr<Object>>& flattenedTree);
	// Depth-first search. Recursive
	void FlattenActive(std::vector<std::shared_ptr<Object>>& flattenedTree);

	// Does not get empty or inactive objects. See FindAllObjectsBySubstring for a function that does
	std::vector<std::shared_ptr<Object>> FindObjectsBySubstring(std::wstring name);
	// Also returns empty and inactive objects. See FindObjectsBySubstring for a function that doesn't
	std::vector<std::shared_ptr<Object>> FindAllObjectsBySubstring(std::wstring name);

	// Returns the first object that matches the name, searched depth-first, including inactive objects
	std::shared_ptr<Object> FindFirstObjectByName(std::wstring name);
	// Returns the first active object that matches the name, searched depth-first, excluding inactive objects
	std::shared_ptr<Object> FindFirstActiveObjectByName(std::wstring name);

protected:
	std::wstring name;
	std::shared_ptr<Mesh> mesh = nullptr;

	std::vector<std::shared_ptr<Object>> children{};
	std::shared_ptr<Object> parent = nullptr;

	bool active = true;
	bool isScene = false;

public:
	//// Static Object creation functions ////

	static std::shared_ptr<Object> CreateObject(std::wstring name = L"Unnamed Empty Object", std::shared_ptr<Object> parent = nullptr);
	static std::shared_ptr<Object> CreateObject(std::shared_ptr<Mesh> mesh, std::wstring name = L"Unanmed Object", std::shared_ptr<Object> parent = nullptr);
};