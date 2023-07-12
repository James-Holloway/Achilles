#include "Material.h"

Material::Material()
{

}

Material::Material(std::shared_ptr<Shader> _shader)
{
    shader = _shader;
}

Material::Material(const Material& other)
{
    shader = other.shader;
    for (auto texture : other.textures)
    {
        textures.insert({ texture.first, texture.second });
    }
    for (auto aFloat : other.floats)
    {
        floats.insert({ aFloat.first, aFloat.second });
    }
    for (auto vector : other.vectors)
    {
        vectors.insert({ vector.first, vector.second });
    }
}

std::shared_ptr<Texture> Material::GetTexture(std::wstring key)
{
    if (textures.find(key) == textures.end())
        return nullptr;
    return textures[key];
}
float Material::GetFloat(std::wstring key)
{
    if (floats.find(key) == floats.end())
        return 0;
    return floats[key];
}
DirectX::SimpleMath::Vector4 Material::GetVector(std::wstring key)
{
    if (vectors.find(key) == vectors.end())
        return DirectX::SimpleMath::Vector4(0,0,0,0);
    return vectors[key];
}

bool Material::HasTexture(std::wstring key)
{
    return textures.find(key) != textures.end();
}
bool Material::HasFloat(std::wstring key)
{
    return floats.find(key) != floats.end();
}
bool Material::HasVector(std::wstring key)
{
    return vectors.find(key) != vectors.end();
}

void Material::SetTexture(std::wstring key, std::shared_ptr<Texture> texture)
{
    textures[key] = texture;
}
void Material::SetFloat(std::wstring key, float _float)
{
    floats[key] = _float;
}
void Material::SetVector(std::wstring key, DirectX::SimpleMath::Vector4 vector)
{
    vectors[key] = vector;
}