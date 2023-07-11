#include "Object.h"
#include "Mesh.h"
#include "Shader.h"
#include "Application.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "CommandQueue.h"
#include "CommandList.h"

using namespace DirectX::SimpleMath;

//// Private constructor & destructor functions ////

Object::Object(std::wstring _name) : name(_name)
{
}

Object::~Object()
{

}

std::shared_ptr<Object> Object::Clone(std::shared_ptr<Object> newParent)
{
    if (newParent == nullptr)
        newParent = GetParent();

    std::shared_ptr<Object> clone = std::make_shared<Object>(name + L" (Clone)");
    clone->SetParent(newParent);
    clone->SetLocalPosition(GetLocalPosition());
    clone->SetLocalRotation(GetLocalRotation());
    clone->SetLocalScale(GetLocalScale());
    for (uint32_t i = 0; i < knits.size(); i++)
    {
        clone->SetKnit(i, Knit(knits[i]));
    }
    clone->SetActive(IsActive());

    for (auto child : GetChildren())
    {
        child->Clone(clone);
    }

    return clone;
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


//// Knit, mesh and material functions ////

Knit& Object::GetKnit(uint32_t index)
{
    if (knits.size() <= index)
        throw std::exception("Object did not have that many knits");

    return knits[index];
}
void Object::SetKnit(uint32_t index, Knit knit)
{
    if (knits.size() <= index)
        knits.resize(index + 1);

    knits[index] = knit;
}
uint32_t Object::GetKnitCount()
{
    return (uint32_t)knits.size();
}
void Object::ResizeKnits(uint32_t newSize)
{
    knits.resize(newSize);
}

std::shared_ptr<Mesh> Object::GetMesh(uint32_t index)
{
    if (knits.size() <= index)
        throw std::exception("Object did not have that many meshes");

    return knits[index].mesh;
}
void Object::SetMesh(uint32_t index, std::shared_ptr<Mesh> _mesh)
{
    if (knits.size() <= index)
        knits.resize(index + 1);

    knits[index].mesh = _mesh;
    if (_mesh != nullptr)
        knits[index].material.shader = _mesh->shader;
}

Material& Object::GetMaterial(uint32_t index)
{
    if (knits.size() <= index)
        throw std::exception("Object did not have that many materials");

    return knits[index].material;
}
void Object::SetMaterial(uint32_t index, Material _material)
{
    if (knits.size() <= index)
        knits.resize(index + 1);

    knits[index].material = _material;
}

//// Empty / Active functions ////

bool Object::IsEmpty()
{
    for (Knit knit : knits)
    {
        if (knit.mesh != nullptr)
            return false;
    }
    return true;
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
    if (_object == nullptr)
        return _object;
    if (_object == shared_from_this())
        throw std::exception("An Object cannot be a child of itself");

    // Only add the child if we haven't already
    if (!Contains<std::shared_ptr<Object>>(children, _object))
    {
        children.push_back(_object);
    }
    if (_object->GetParent() != shared_from_this())
    {
        _object->SetParent(shared_from_this());
    }
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

std::shared_ptr<Object>& Object::operator[](size_t&& index)
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
    if (parent != nullptr)
    {
        parent->AddChild(shared_from_this());
    }
    SetWorldMatrixDirty();

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


//// Position, rotation, scale and matrix functions ////

Matrix Object::GetLocalMatrix()
{
    if (dirtyMatrix)
        ConstructMatrix();
    return matrix;
}
Vector3 Object::GetLocalPosition()
{
    return position;
}
Vector3 Object::GetLocalRotation()
{
    return rotation;
}
Vector3 Object::GetLocalScale()
{
    return scale;
}

Matrix Object::GetWorldMatrix()
{
    if (dirtyWorldMatrix)
        ConstructWorldMatrix();
    return worldMatrix;
}
Vector3 Object::GetWorldPosition()
{
    Vector3 position, scale;
    Quaternion quaternion;
    GetWorldMatrix().Decompose(scale, quaternion, position);
    return position;
}
Vector3 Object::GetWorldRotation()
{
    Vector3 position, scale;
    Quaternion quaternion;
    GetWorldMatrix().Decompose(scale, quaternion, position);
    return quaternion.ToEuler();
}
Vector3 Object::GetWorldScale()
{
    Vector3 position, scale;
    Quaternion quaternion;
    GetWorldMatrix().Decompose(scale, quaternion, position);
    return scale;
}

void Object::SetLocalPosition(Vector3 _position)
{
    if (isScene)
        return;
    position = _position;
    dirtyMatrix = true;
    SetWorldMatrixDirty();
}
void Object::SetLocalRotation(Vector3 _rotation)
{
    if (isScene)
        return;
    rotation = _rotation;
    dirtyMatrix = true;
    SetWorldMatrixDirty();
}
void Object::SetLocalScale(Vector3 _scale)
{
    if (isScene)
        return;
    scale = _scale;
    dirtyMatrix = true;
    SetWorldMatrixDirty();
}
void Object::SetLocalMatrix(Matrix _matrix)
{
    Vector3 matrixPos, matrixScale;
    Quaternion matrixQuaternion;
    _matrix.Transpose().Decompose(matrixScale, matrixQuaternion, matrixPos);
    matrix = _matrix;
    position = matrixPos;
    rotation = matrixQuaternion.ToEuler();
    scale = matrixScale;
    dirtyMatrix = true;
    SetWorldMatrixDirty();
}

void Object::SetWorldPosition(Vector3 _position)
{
    if (parent == nullptr)
    {
        SetLocalPosition(_position);
        return;
    }
    Vector3 parentPosition, parentScale;
    Quaternion parentQuaternion;
    parent->GetWorldMatrix().Decompose(parentScale, parentQuaternion, parentPosition);
    SetLocalPosition(parentPosition - _position);
}
void Object::SetWorldRotation(Vector3 _rotation)
{
    if (parent == nullptr)
    {
        SetLocalRotation(_rotation);
        return;
    }
    Vector3 parentPosition, parentScale;
    Quaternion parentQuaternion;
    parent->GetWorldMatrix().Decompose(parentScale, parentQuaternion, parentPosition);
    SetLocalRotation(parentQuaternion.ToEuler() - _rotation);
}
void Object::SetWorldScale(Vector3 _scale)
{
    if (parent == nullptr)
    {
        SetLocalScale(_scale);
        return;
    }
    Vector3 parentPosition, parentScale;
    Quaternion parentQuaternion;
    parent->GetWorldMatrix().Decompose(parentScale, parentQuaternion, parentPosition);
    SetLocalPosition(_scale / parentScale);
}
void Object::SetWorldMatrix(Matrix _matrix)
{
    if (parent == nullptr)
    {
        SetLocalMatrix(_matrix);
        return;
    }
    SetLocalMatrix(parent->GetWorldMatrix() - _matrix);
}

void Object::ConstructMatrix()
{
    matrix = Matrix::CreateScale(scale) * (Matrix::CreateFromYawPitchRoll(rotation) * Matrix::CreateTranslation(position));
    dirtyMatrix = false;
}
void Object::ConstructWorldMatrix()
{
    if (isScene)
        worldMatrix = Matrix::Identity;
    else if (parent != nullptr)
        worldMatrix = GetLocalMatrix() * parent->GetWorldMatrix();
    else
        worldMatrix = GetLocalMatrix();

    dirtyWorldMatrix = false;
}
void Object::SetWorldMatrixDirty()
{
    dirtyWorldMatrix = true;

    for (std::shared_ptr<Object> child : children)
    {
        if (child != nullptr)
            child->SetWorldMatrixDirty();
    }
}


//// Static Object creation functions ////

std::shared_ptr<Object> Object::CreateObject(std::wstring name, std::shared_ptr<Object> parent)
{
    std::shared_ptr<Object> object = std::make_shared<Object>(name);
    object->SetParent(parent);

    return object;
}

std::shared_ptr<Object> Object::CreateObjectsFromSceneNode(aiScene* scene, aiNode* node, std::shared_ptr<Object> parent, std::shared_ptr<Shader> shader)
{
    std::shared_ptr<Object> thisObject = CreateObject(StringToWString(node->mName.C_Str()), parent);

    MeshCreation createFunc = shader->meshCreateCallback;
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* inMesh = scene->mMeshes[node->mMeshes[i]];
        thisObject->SetKnit(i, Knit{}); // resize the knit vector so we can get the material directtly
        std::shared_ptr<Mesh> mesh = createFunc(scene, inMesh, shader, thisObject->GetMaterial(i));
        thisObject->SetMesh(i, mesh);
    }

    aiMatrix4x4 transform = node->mTransformation;
    Matrix transformMatrix = {
        transform.a1, transform.a2, transform.a3, transform.a4,
        transform.b1, transform.b2, transform.b3, transform.b4,
        transform.c1, transform.c2, transform.c3, transform.c4,
        transform.d1, transform.d2, transform.d3, transform.d4,
    };

    // If we are a direct child of the root node then scale down into meters
    if (node->mParent != nullptr && node->mParent == scene->mRootNode)
    {
        transformMatrix *= Matrix::CreateScale(0.01f);
    }
    thisObject->SetLocalMatrix(transformMatrix);

    thisObject->SetName(StringToWString(node->mName.C_Str()));

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        CreateObjectsFromSceneNode(scene, node->mChildren[i], thisObject, shader);
    }
    return thisObject;
}

std::shared_ptr<Object> Object::CreateObjectsFromScene(aiScene* scene, std::shared_ptr<Shader> shader)
{
    GetCreationCommandList(); // Create the command list that we will execute later

    std::shared_ptr<Object> objectTree = CreateObjectsFromSceneNode(scene, scene->mRootNode, nullptr, shader);
    if (objectTree->GetName() == L"RootNode")
    {
        if (scene->mName.length != 0 && scene->mName.C_Str() != "")
            objectTree->SetName(StringToWString(scene->mName.C_Str()));
        else
            objectTree->SetName(L"Unnamed Object");
    }

    // If there is only one child then remove the RootNode
    if (objectTree->GetChildren().size() == 1)
    {
        objectTree = objectTree->GetChildren()[0];
        objectTree->SetParent(nullptr);
    }

    ExecuteCreationCommandList();

    return objectTree;
}

std::shared_ptr<Object> Object::CreateObjectsFromFile(std::wstring filePath, std::shared_ptr<Shader> shader)
{
    static Assimp::Importer importer;

    std::string filePathA = WStringToString(filePath);

    aiScene* scene = const_cast<aiScene*>(importer.ReadFile(filePathA, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_SplitLargeMeshes));
    if (scene == nullptr)
    {
        OutputDebugStringAFormatted("Model importing (%s) failed: %s\n", filePathA, importer.GetErrorString());
        return nullptr;
    }

    std::wstring fileName = std::filesystem::path(filePath).replace_extension().filename();
    
    std::shared_ptr<Object> object = CreateObjectsFromScene(scene, shader);
    if (object->GetName() == L"Unnamed Object")
        object->SetName(fileName);

    return object;
}

std::shared_ptr<Object> Object::CreateObjectsFromContentFile(std::wstring file, std::shared_ptr<Shader> shader)
{
    return CreateObjectsFromFile(GetContentDirectoryW() + L"models/" + file, shader);
}

std::shared_ptr<CommandList> Object::GetCreationCommandList()
{
    if (currentCreationCommandQueue == nullptr)
        currentCreationCommandQueue = Application::GetNewCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    if (currentCreationCommandList == nullptr)
        currentCreationCommandList = currentCreationCommandQueue->GetCommandList();

    return currentCreationCommandList;
}

void Object::ExecuteCreationCommandList()
{
    if (currentCreationCommandQueue == nullptr || currentCreationCommandList == nullptr)
        return; //throw std::exception("Command queue or list was null, cannot execute");

    currentCreationCommandQueue->ExecuteCommandList(currentCreationCommandList);
    currentCreationCommandList = nullptr;
}