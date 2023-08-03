#pragma once

#include "Object.h"

class Scene
{
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

protected:
    bool isActive = false;
    std::shared_ptr<Object> objectTree;
    std::wstring name;
};

struct {
    bool operator()(std::shared_ptr<Scene>& a, std::shared_ptr<Scene>& b) const
    {
        return a->GetName() < b->GetName();
    }
} SceneSharedPtrNameSort;