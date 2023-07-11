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
