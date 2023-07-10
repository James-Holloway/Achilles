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

	std::shared_ptr<Shader> shader;
	std::map<std::wstring, std::shared_ptr<Texture>> textures;
	std::map<std::wstring, float> floats;
	std::map<std::wstring, DirectX::SimpleMath::Vector4> vectors;
};