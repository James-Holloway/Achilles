#pragma once

#include "Object.h"
#include "SpriteCommon.h"
#include "shaders/SpriteUnlit.h"

using DirectX::SimpleMath::Color;

class SpriteObject : public Object
{
public:
    SpriteObject(std::wstring _name = Object::DefaultName);
    virtual ~SpriteObject();

    virtual Color GetSpriteColor();
    virtual void SetSpriteColor(Color color);

    virtual bool ShouldDraw(DirectX::BoundingFrustum frustum) override;
    bool GetEditorSprite();
    void SetEditorSprite(bool _editorSprite);

    std::shared_ptr<Texture> GetSpriteTexture();
    void SetSpriteTexture(std::shared_ptr<Texture> _spriteTexture);

    SpriteShape GetSpriteShape();
    void SetSpriteShape(SpriteShape _shape);
    std::shared_ptr<Mesh> GetSpriteMesh(std::shared_ptr<CommandList> commandList);

protected:
    Color spriteColor{ 1, 1, 1, 1 };
    bool editorSprite = false;
    SpriteShape shape = SpriteShape::Square;
    std::shared_ptr<Texture> spriteTexture;
};
