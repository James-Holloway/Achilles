#include "Material.h"

Material::Material()
{

}

Material::Material(std::shared_ptr<Shader> _shader)
{
	shader = _shader;
}
