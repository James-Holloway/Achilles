#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "ObjectTag.h"
#include "Material.h"
#include "Knit.h"

class Mesh;
class Object;
class LightObject;
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiLight;
class CommandQueue;
class CommandList;

// Return false to end traverse early
typedef bool (CALLBACK* TraverseObject)(std::shared_ptr<Object> object);
// Called just before returning up the depth
typedef void (CALLBACK* TraverseObjectUp)(std::shared_ptr<Object> object);

class Object : public std::enable_shared_from_this<Object>
{
    friend class Scene; // Scene can set isScene and instantiate an empty constructor
    friend class Achilles; // Achilles can access currentCreationCommandQueue
public:
    inline static std::wstring DefaultName = L"Unnamed Object";
    //// Public constructors & destructor functions ////

    // Should not be used. Use CreateObject instead!
    Object(std::wstring _name = DefaultName);
    virtual ~Object();
    virtual std::shared_ptr<Object> Clone(std::shared_ptr<Object> newParent = nullptr);

public:
    //// Name functions ////

    std::wstring GetName();
    void SetName(std::wstring _name);


    //// Tag functions ////

    ObjectTag GetTags();
    bool HasTag(ObjectTag _tag);
    void SetTags(ObjectTag _tags);
    void AddTag(ObjectTag _tags);
    void RemoveTags(ObjectTag _tags);


    //// Knit, mesh and material functions ////

    Knit& GetKnit(uint32_t index = 0);
    void SetKnit(uint32_t index, Knit knit);
    uint32_t GetKnitCount();
    // Resizes the vector of knits. Setting lower than knit count will delete knits
    void ResizeKnits(uint32_t newSize);

    std::shared_ptr<Mesh> GetMesh(uint32_t index = 0);
    // Also sets the material's shader to the set mesh's shader. Use SetMaterial after SetMesh to allow overriding
    void SetMesh(uint32_t index, std::shared_ptr<Mesh> _mesh);

    Material& GetMaterial(uint32_t index = 0);
    void SetMaterial(uint32_t index, Material _material);


    //// Empty / Active functions ////

    bool IsEmpty();
    bool IsActive();
    void SetActive(bool _active);


    //// Shadow States ////

    bool CastsShadows();
    bool ReceivesShadows();
    void SetCastsShadows(bool _castShadows);
    void SetReceiveShadows(bool _receiveShadows);


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
    // Keeps the world transform from before the new parent was set
    bool SetParentKeepTransform(std::shared_ptr<Object> newParent);


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
    // Verifies if this object belongs to a scene
    bool IsOrphaned();


    //// Position, rotation, scale and matrix functions ////

    DirectX::SimpleMath::Matrix GetLocalMatrix();
    DirectX::SimpleMath::Vector3 GetLocalPosition();
    DirectX::SimpleMath::Quaternion GetLocalRotation();
    DirectX::SimpleMath::Vector3 GetLocalScale();

    DirectX::SimpleMath::Matrix GetWorldMatrix();
    DirectX::SimpleMath::Matrix GetInverseWorldMatrix();
    DirectX::SimpleMath::Vector3 GetWorldPosition();
    DirectX::SimpleMath::Quaternion GetWorldRotation();
    DirectX::SimpleMath::Vector3 GetWorldScale();

    void SetLocalPosition(DirectX::SimpleMath::Vector3 _position);
    void SetLocalRotation(DirectX::SimpleMath::Quaternion _rotation);
    void SetLocalScale(DirectX::SimpleMath::Vector3 _scale);
    void SetLocalMatrix(DirectX::SimpleMath::Matrix _matrix);

    void SetWorldPosition(DirectX::SimpleMath::Vector3 _position);
    void SetWorldRotation(DirectX::SimpleMath::Quaternion _rotation);
    void SetWorldScale(DirectX::SimpleMath::Vector3 _scale);
    void SetWorldMatrix(DirectX::SimpleMath::Matrix _matrix);

protected:
    void ConstructMatrix();
    void ConstructWorldMatrix();
    // Called by a parent when its world matrix changes. Recursive
    void SetWorldMatrixDirty();

protected:
    //// Member variables ////

    std::wstring name;

    ObjectTag tags = ObjectTag::None;

    std::vector<Knit> knits;

    std::vector<std::shared_ptr<Object>> children{};
    std::weak_ptr<Object> parent{};

    bool active = true;
    bool isScene = false;

    bool castShadows = true;
    bool receiveShadows = true;

    DirectX::SimpleMath::Vector3 position {0, 0, 0};
    DirectX::SimpleMath::Quaternion rotation {0, 0, 0, 1};
    DirectX::SimpleMath::Vector3 scale {1, 1, 1};
    DirectX::SimpleMath::Matrix matrix;
    DirectX::SimpleMath::Matrix worldMatrix;
    DirectX::SimpleMath::Matrix inverseWorldMatrix;
    bool dirtyMatrix = true;
    bool dirtyWorldMatrix = true;

public:
    //// Static Object creation functions ////

    static std::shared_ptr<Object> CreateObject(std::wstring name = DefaultName, std::shared_ptr<Object> parent = nullptr);
    static std::shared_ptr<LightObject> CreateLightObject(std::wstring name = DefaultName, std::shared_ptr<Object> parent = nullptr);


    // Create objects from an aiScene

    // Create a light object from the scene
    static std::shared_ptr<LightObject> CreateLightObjectFromSceneNode(aiScene* scene, aiNode* node, aiLight* light, std::shared_ptr<Object> parent);
    // Returns one object or nullptr. Child meshes are parented under one object with the scene name
    static std::shared_ptr<Object> CreateObjectsFromSceneNode(aiScene* scene, aiNode* node, std::shared_ptr<Object> parent, std::shared_ptr<Shader> shader);
    static std::shared_ptr<Object> CreateObjectsFromScene(aiScene* scene, std::shared_ptr<Shader> shader);
    static std::shared_ptr<Object> CreateObjectsFromFile(std::wstring filePath, std::shared_ptr<Shader> shader);
    static std::shared_ptr<Object> CreateObjectsFromContentFile(std::wstring file, std::shared_ptr<Shader> shader);

    static std::shared_ptr<CommandList> GetCreationCommandList();
    static void ExecuteCreationCommandList();

protected:
    inline static std::shared_ptr<CommandQueue> currentCreationCommandQueue = nullptr;
    inline static std::shared_ptr<CommandList> currentCreationCommandList = nullptr;
};
