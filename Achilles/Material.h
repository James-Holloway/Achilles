#pragma once

#include <memory>
#include <string>
#include <map>
#include "directxtk12/SimpleMath.h"

class Texture;
class Shader;

// Materials hold a shader and some properties held by maps (dictionaries), keyed by a wstring, which hold texture pointers, floats and vectors
// Converting these properties into commands requires action by the shader
class Material
{
public:
    Material();
    Material(std::shared_ptr<Shader> _shader);
    // Clone the material
    Material(const Material& other);

    std::shared_ptr<Texture> GetTexture(std::wstring key);
    float GetFloat(std::wstring key);
    DirectX::SimpleMath::Vector4 GetVector(std::wstring key);

    bool HasTexture(std::wstring key);
    bool HasFloat(std::wstring key);
    bool HasVector(std::wstring key);

    void SetTexture(std::wstring key, std::shared_ptr<Texture> texture);
    void SetFloat(std::wstring key, float _float);
    void SetVector(std::wstring key, DirectX::SimpleMath::Vector4 vector);

    std::wstring name = L"Unnamed Material";
    std::shared_ptr<Shader> shader;
    std::map<std::wstring, std::shared_ptr<Texture>> textures;
    std::map<std::wstring, float> floats;
    std::map<std::wstring, DirectX::SimpleMath::Vector4> vectors;
    bool transparency = false;
};