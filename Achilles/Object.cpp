#include "Object.h"
#include "Mesh.h"
#include "Shader.h"
#include "Application.h"
#include "MathHelpers.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "CommandQueue.h"
#include "CommandList.h"
#include "LightObject.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

//// Private constructor & destructor functions ////

Object::Object(std::wstring _name) : name(_name)
{
    AddTag(ObjectTag::Mesh);
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


//// Tag functions ////

ObjectTag Object::GetTags()
{
    return tags;
}
bool Object::HasTag(ObjectTag _tag)
{
    return (size_t)(tags & _tag) > 0;
}
void Object::SetTags(ObjectTag _tags)
{
    tags = _tags;
}
void Object::AddTag(ObjectTag _tags)
{
    tags |= _tags;
}
void Object::RemoveTags(ObjectTag _tags)
{
    tags &= ~_tags;
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


//// Shadow States ////
bool Object::CastsShadows()
{
    return castShadows;
}
bool Object::ReceivesShadows()
{
    return receiveShadows;
}
void Object::SetCastsShadows(bool _castShadows)
{
    castShadows = _castShadows;
}
void Object::SetReceiveShadows(bool _receiveShadows)
{
    receiveShadows = _receiveShadows;
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
    return parent.lock();
}

bool Object::SetParent(std::shared_ptr<Object> newParent)
{
    if (isScene)
        return false;
    if (newParent == shared_from_this())
        throw std::exception("An Object cannot be a parent of itself");
    if (GetParent() != nullptr)
    {
        GetParent()->RemoveChild(shared_from_this());
    }
    parent = newParent;
    if (newParent != nullptr)
    {
        newParent->AddChild(shared_from_this());
    }
    SetWorldMatrixDirty();

    return true;
}

bool Object::SetParentKeepTransform(std::shared_ptr<Object> newParent)
{
    Matrix matrix = GetWorldMatrix();
    bool succeeded = SetParent(newParent);
    if (succeeded)
    {
        SetWorldMatrix(matrix);
    }

    return succeeded;
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
    if (IsActive() || isScene)
    {
        if (!isScene)
            flattenedTree.push_back(shared_from_this());

        for (std::shared_ptr<Object> child : children)
        {
            child->FlattenActive(flattenedTree);
        }
    }
}

std::vector<std::shared_ptr<Object>> Object::FindObjectsBySubstring(std::wstring name)
{
    std::vector<std::shared_ptr<Object>> objects;
    FlattenActive(objects);

    std::vector<std::shared_ptr<Object>> matchedObjects;
    for (std::shared_ptr<Object> object : objects)
    {
        if (object == nullptr)
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
        if (object == nullptr)
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
        if (object == nullptr)
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
        if (object == nullptr)
            continue;

        if (name == object->GetName())
            return object;
    }
    return nullptr;
}

bool Object::IsOrphaned()
{
    std::shared_ptr<Object> _parent = GetParent();
    if (_parent == nullptr)
        return true;

    if (_parent->isScene)
        return false;

    return _parent->IsOrphaned();
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
Quaternion Object::GetLocalRotation()
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
DirectX::SimpleMath::Matrix Object::GetInverseWorldMatrix()
{
    if (dirtyWorldMatrix)
        ConstructWorldMatrix();
    return inverseWorldMatrix;
}

Vector3 Object::GetWorldPosition()
{
    Vector3 position, scale;
    Quaternion quaternion;
    GetWorldMatrix().Decompose(scale, quaternion, position);
    return position;
}
Quaternion Object::GetWorldRotation()
{
    Vector3 position, scale;
    Quaternion quaternion;
    GetWorldMatrix().Decompose(scale, quaternion, position);
    return quaternion;
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
void Object::SetLocalRotation(Quaternion _rotation)
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
    _matrix.Decompose(matrixScale, matrixQuaternion, matrixPos);
    matrix = _matrix;
    position = matrixPos;
    rotation = matrixQuaternion;
    scale = matrixScale;
    dirtyMatrix = true;
    SetWorldMatrixDirty();
}

void Object::SetWorldPosition(Vector3 _position)
{
    std::shared_ptr<Object> parent = GetParent();
    if (parent == nullptr)
    {
        SetLocalPosition(_position);
        return;
    }

    SetLocalPosition(Multiply(parent->GetInverseWorldMatrix(), _position));
}
void Object::SetWorldRotation(Quaternion _rotation)
{
    if (GetParent() == nullptr)
    {
        SetLocalRotation(_rotation);
        return;
    }
    Quaternion parentRotation = GetParent()->GetWorldRotation();
    parentRotation = Inverse(parentRotation);
    SetLocalRotation(_rotation * parentRotation);
}
void Object::SetWorldScale(Vector3 _scale)
{
    if (GetParent() == nullptr)
    {
        SetLocalScale(_scale);
        return;
    }
    Vector3 parentPosition, parentScale;
    Quaternion parentQuaternion;
    GetParent()->GetWorldMatrix().Decompose(parentScale, parentQuaternion, parentPosition);
    SetLocalScale(_scale / parentScale);
}
void Object::SetWorldMatrix(Matrix _matrix)
{
    if (GetParent() == nullptr)
    {
        SetLocalMatrix(_matrix);
        return;
    }
    SetLocalMatrix(GetParent()->GetWorldMatrix().Invert() * _matrix);
}

DirectX::BoundingOrientedBox Object::GetBoundingBox()
{
    if (dirtyBoundingBox)
        CalculateBoundingBox();

    return boundingBox;
}

DirectX::BoundingOrientedBox Object::GetWorldBoundingBox()
{
    BoundingOrientedBox obb = GetBoundingBox();
    obb.Extents = obb.Extents * GetWorldScale();
    obb.Center = obb.Center + GetWorldPosition();
    obb.Orientation = GetWorldRotation();
    return obb;
}

void Object::SetBoundingBox(DirectX::BoundingOrientedBox box)
{
    boundingBox = box;
    dirtyBoundingBox = false;
}

void Object::SetBoundingBoxDirty()
{
    dirtyBoundingBox = true;
}

bool Object::ShouldDraw(DirectX::BoundingFrustum frustum)
{
    return frustum.Contains(GetWorldBoundingBox()) != DirectX::ContainmentType::DISJOINT;
}

void Object::ConstructMatrix()
{
    // SRT
    matrix = (Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation)) * Matrix::CreateTranslation(position);
    dirtyMatrix = false;
}
void Object::ConstructWorldMatrix()
{
    if (isScene)
        worldMatrix = Matrix::Identity;
    else if (GetParent() != nullptr)
        worldMatrix = GetLocalMatrix() * GetParent()->GetWorldMatrix();
    else
        worldMatrix = GetLocalMatrix();

    inverseWorldMatrix = worldMatrix.Invert();

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

void Object::CalculateBoundingBox()
{
    BoundingBox newBB;
    newBB.Extents = Vector3(0.1f, 0.1f, 0.1f);

    for (uint32_t i = 0; i < GetKnitCount(); i++)
    {
        BoundingBox bb = GetMesh(i)->GetBoundingBox();
        BoundingBox::CreateMerged(newBB, newBB, bb);
    }

    BoundingOrientedBox::CreateFromBoundingBox(boundingBox, newBB);
    dirtyBoundingBox = false;
}


//// Static Object creation functions ////

std::shared_ptr<Object> Object::CreateObject(std::wstring name, std::shared_ptr<Object> parent)
{
    std::shared_ptr<Object> object = std::make_shared<Object>(name);
    object->SetParent(parent);

    return object;
}

std::shared_ptr<LightObject> Object::CreateLightObject(std::wstring name, std::shared_ptr<Object> parent)
{
    std::shared_ptr<LightObject> lightObject = std::make_shared<LightObject>(name);
    lightObject->SetParent(parent);
    return lightObject;
}

static Color GetColorFromLight(aiLight* light, float& strength)
{
    aiColor3D col = light->mColorDiffuse;
    strength = std::max<float>({ col.r, col.g, col.b });
    Vector3 colv3 = Vector3(col.r, col.g, col.b) / strength;
    strength /= 1000.0f;
    return Color(colv3.x, colv3.y, colv3.z, 1);
}

static void GetAttenuationFromLight(aiLight* light, PointLight outLight)
{
    // Divide linear and quadratic components by 2 to compensate for using a attenuation constant of 1
    if (light->mAttenuationConstant == 0.0f)
    {
        outLight.ConstantAttenuation = 1.0f;
        outLight.LinearAttenuation = light->mAttenuationLinear / 2.0f;
        outLight.QuadraticAttenuation = light->mAttenuationQuadratic / 2.0f;
    }
    else // only happens if attenuation is constant (and linear + quadratic are 0)
    {
        outLight.ConstantAttenuation = light->mAttenuationConstant;
        outLight.LinearAttenuation = light->mAttenuationLinear;
        outLight.QuadraticAttenuation = light->mAttenuationQuadratic;
    }
}

std::shared_ptr<LightObject> Object::CreateLightObjectFromSceneNode(aiScene* scene, aiNode* node, aiLight* light, std::shared_ptr<Object> parent)
{
    bool directChildOfRoot = node->mParent != nullptr && node->mParent == scene->mRootNode;
    std::shared_ptr<LightObject> lightObject = Object::CreateLightObject(StringToWString(node->mName.C_Str()), parent);

    Vector3 lightDirection = Vector3(light->mDirection.x, light->mDirection.y, light->mDirection.z);

    switch (light->mType)
    {
    case aiLightSource_POINT:
    {
        PointLight point{};
        point.Color = GetColorFromLight(light, point.Strength);

        GetAttenuationFromLight(light, point);

        lightObject->AddLight(point);
        break;
    }
    case aiLightSource_SPOT:
    {
        SpotLight spot{};
        spot.Light.Color = GetColorFromLight(light, spot.Light.Strength);

        GetAttenuationFromLight(light, spot.Light);

        spot.InnerSpotAngle = light->mAngleInnerCone;
        spot.OuterSpotAngle = light->mAngleOuterCone;

        lightObject->AddLight(spot);
        break;
    }
    case aiLightSource_DIRECTIONAL:
    {
        DirectionalLight directional{};
        directional.Color = GetColorFromLight(light, directional.Strength);
        directional.Strength *= 1000; // undo division by 1000 as Blender doesn't use Watts for sun lights
        lightObject->AddLight(directional);
        break;
    }
    case aiLightSource_AREA: // can't handle, we don't support area lights
    case aiLightSource_AMBIENT: // don't handle, we have our own ambient colour
    default:
        break;
    }

    aiMatrix4x4 transform = node->mTransformation;
    Matrix transformMatrix = {
        transform.a1, transform.a2, transform.a3, transform.a4,
        transform.b1, transform.b2, transform.b3, transform.b4,
        transform.c1, transform.c2, transform.c3, transform.c4,
        transform.d1, transform.d2, transform.d3, transform.d4,
    };
    Vector3 scale, position;
    Quaternion rotation;

    transformMatrix.Transpose().Decompose(scale, rotation, position);

    // If we are a direct child of the root node then scale down into meters
    if (directChildOfRoot)
    {
        transformMatrix /= 100.0f;
    }

    transformMatrix.Transpose().Decompose(scale, rotation, position);

    lightObject->SetLocalScale(scale);
    lightObject->SetLocalPosition(position);

    Quaternion swizzledRotation;
    swizzledRotation.x = rotation.z;
    swizzledRotation.y = -rotation.x;
    swizzledRotation.z = rotation.w;
    swizzledRotation.w = rotation.y;

    lightObject->SetLocalRotation(swizzledRotation);

    return lightObject;
}

std::shared_ptr<Object> Object::CreateObjectsFromSceneNode(aiScene* scene, aiNode* node, std::shared_ptr<Object> parent, std::shared_ptr<Shader> shader, std::wstring filePath)
{
    std::shared_ptr<Object> thisObject;

    // Check if this node is a light
    for (uint32_t i = 0; i < scene->mNumLights; i++)
    {
        if (scene->mLights[i]->mName == node->mName)
        {
            // This node is a light
            thisObject = std::dynamic_pointer_cast<Object>(CreateLightObjectFromSceneNode(scene, node, scene->mLights[i], parent));
            for (uint32_t i = 0; i < node->mNumChildren; i++)
            {
                CreateObjectsFromSceneNode(scene, node->mChildren[i], thisObject, shader, filePath);
            }
            return thisObject;
        }
    }

    // The object wasn't a light, so create a mesh
    thisObject = CreateObject(StringToWString(node->mName.C_Str()), parent);

    MeshCreation createFunc = shader->meshCreateCallback;
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* inMesh = scene->mMeshes[node->mMeshes[i]];
        thisObject->SetKnit(i, Knit{}); // resize the knit vector so we can get the material directtly
        std::shared_ptr<Mesh> mesh = createFunc(scene, node, inMesh, shader, thisObject->GetMaterial(i), filePath);
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
        transformMatrix /= 100.0f;
    }
    thisObject->SetLocalMatrix(transformMatrix.Transpose());

    thisObject->SetName(StringToWString(node->mName.C_Str()));

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        CreateObjectsFromSceneNode(scene, node->mChildren[i], thisObject, shader, filePath);
    }
    return thisObject;
}

std::shared_ptr<Object> Object::CreateObjectsFromScene(aiScene* scene, std::shared_ptr<Shader> shader, std::wstring filePath)
{
    GetCreationCommandList(); // Create the command list that we will execute later

    // https://github.com/assimp/assimp/issues/849#issuecomment-875475292
    // Rotates the mesh based on import direction
    if (scene->mMetaData)
    {
        int32_t UpAxis = 1, UpAxisSign = 1, FrontAxis = 2, FrontAxisSign = 1, CoordAxis = 0, CoordAxisSign = 1;
        double UnitScaleFactor = 1.0;
        for (unsigned MetadataIndex = 0; MetadataIndex < scene->mMetaData->mNumProperties; ++MetadataIndex)
        {
            if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "UpAxis") == 0)
            {
                scene->mMetaData->Get<int32_t>(MetadataIndex, UpAxis);
            }
            if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "UpAxisSign") == 0)
            {
                scene->mMetaData->Get<int32_t>(MetadataIndex, UpAxisSign);
            }
            if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "FrontAxis") == 0)
            {
                scene->mMetaData->Get<int32_t>(MetadataIndex, FrontAxis);
            }
            if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "FrontAxisSign") == 0)
            {
                scene->mMetaData->Get<int32_t>(MetadataIndex, FrontAxisSign);
            }
            if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "CoordAxis") == 0)
            {
                scene->mMetaData->Get<int32_t>(MetadataIndex, CoordAxis);
            }
            if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "CoordAxisSign") == 0)
            {
                scene->mMetaData->Get<int32_t>(MetadataIndex, CoordAxisSign);
            }
            if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "UnitScaleFactor") == 0)
            {
                scene->mMetaData->Get<double>(MetadataIndex, UnitScaleFactor);
            }
        }

        aiVector3D upVec, forwardVec, rightVec;

        upVec[UpAxis] = UpAxisSign * (float)UnitScaleFactor;
        forwardVec[FrontAxis] = FrontAxisSign * (float)UnitScaleFactor;
        rightVec[CoordAxis] = CoordAxisSign * (float)UnitScaleFactor;

        aiMatrix4x4 mat(rightVec.x, rightVec.y, rightVec.z, 0.0f,
            upVec.x, upVec.y, upVec.z, 0.0f,
            forwardVec.x, forwardVec.y, forwardVec.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
        scene->mRootNode->mTransformation = mat;
    }

    std::shared_ptr<Object> objectTree = CreateObjectsFromSceneNode(scene, scene->mRootNode, nullptr, shader, filePath);
    if (objectTree->GetName() == L"RootNode")
    {
        if (scene->mName.length != 0 && scene->mName.C_Str() != "")
            objectTree->SetName(StringToWString(scene->mName.C_Str()));
        else
            objectTree->SetName(DefaultName);
    }

    // If there is only one child then remove the RootNode
    if (objectTree->GetChildren().size() == 1)
    {
        std::shared_ptr<Object> child = objectTree->GetChildren()[0];
        child->SetLocalMatrix(child->GetWorldMatrix()); // apply root transformation
        objectTree = child;
        objectTree->SetParent(nullptr);
    }

    ExecuteCreationCommandList();

    return objectTree;
}

std::shared_ptr<Object> Object::CreateObjectsFromFile(std::wstring filePath, std::shared_ptr<Shader> shader)
{
    static Assimp::Importer importer;

    std::string filePathA = WStringToString(filePath);

    aiScene* scene = const_cast<aiScene*>(importer.ReadFile(filePathA, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_SplitLargeMeshes | aiProcess_MakeLeftHanded | aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_GenBoundingBoxes));
    if (scene == nullptr)
    {
        OutputDebugStringAFormatted("Model importing (%s) failed: %s\n", filePathA, importer.GetErrorString());
        return nullptr;
    }

    std::wstring fileName = std::filesystem::path(filePath).replace_extension().filename();

    std::shared_ptr<Object> object = CreateObjectsFromScene(scene, shader, filePath);
    if (object->GetName() == DefaultName)
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