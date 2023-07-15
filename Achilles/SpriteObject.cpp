#include "SpriteObject.h"
#include "Application.h"

SpriteObject::SpriteObject(std::wstring _name) : Object(_name)
{
    RemoveTags(ObjectTag::Mesh);
    AddTag(ObjectTag::Sprite);
}

SpriteObject::~SpriteObject()
{

}

Color SpriteObject::GetSpriteColor()
{
    return spriteColor;
}
void SpriteObject::SetSpriteColor(Color color)
{
    spriteColor = color;
}

bool SpriteObject::ShouldDraw()
{
    return editorSprite && Application::IsEditor();
}
bool SpriteObject::GetEditorSprite()
{
    return editorSprite;
}
void SpriteObject::SetEditorSprite(bool _editorSprite)
{
    editorSprite = _editorSprite;
}

std::shared_ptr<Texture> SpriteObject::GetSpriteTexture()
{
    return spriteTexture;
}

void SpriteObject::SetSpriteTexture(std::shared_ptr<Texture> _spriteTexture)
{
    spriteTexture = _spriteTexture;
}

SpriteShape SpriteObject::GetSpriteShape()
{
    return shape;
}
void SpriteObject::SetSpriteShape(SpriteShape _shape)
{
    shape = _shape;
}

std::shared_ptr<Mesh> SpriteObject::GetSpriteMesh(std::shared_ptr<CommandList> commandList)
{
    return SpriteUnlit::GetMeshForSpriteShape(commandList, shape);
}
