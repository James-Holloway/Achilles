#pragma once

#include "Object.h"

class Scene : public std::enable_shared_from_this<Scene>
{
    friend struct SceneSharedPtrNameSort;
public:
    Scene(std::wstring _name = L"Scene");
    ~Scene();

    // By adding an ObjectTree to a scene, it will then be deleted when its parent is deleted
    // @param objectTree
    // @param parent is the scene's ObjectTree when nullptr
    void AddObjectToScene(std::shared_ptr<Object> object, std::shared_ptr<Object> parent = nullptr);

    std::shared_ptr<Object> GetObjectTree();

    bool IsActive();
    void SetActive(bool active);

    std::wstring GetName();
    void SetName(std::wstring _name);

    DirectX::BoundingBox GetBoundingBox();
    DirectX::BoundingSphere GetBoundingSphere();

    uint32_t GetSceneIndex();

protected:
    bool isActive = false;
    std::shared_ptr<Object> objectTree;
    std::wstring name;

    uint32_t sceneIndex;

    // Used because we can't used shared_from_this inside constructor
    bool hasSetObjectTreeScene = false;
};

struct {
    bool operator()(std::shared_ptr<Scene>& a, std::shared_ptr<Scene>& b) const
    {
        // Order by name unless they are the same, in which case order by creation order
        if (a->GetName() == b->GetName())
        {
            return a->GetSceneIndex() < b->GetSceneIndex();
        }
        return a->GetName() < b->GetName();
    }
} SceneSharedPtrNameSort;