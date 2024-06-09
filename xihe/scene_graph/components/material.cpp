#include "material.h"

namespace xihe::sg
{

Material::Material(const std::string &name) :
    Component{name}
{}

std::type_index Material::get_type()
{
	return typeid(Material);
}

PbrMaterial::PbrMaterial(const std::string &name) :
    Material{name}
{}

std::type_index PbrMaterial::get_type()
{
	return typeid(PbrMaterial);
}

void PbrMaterial::set_texture_index(const std::string &name, uint32_t texture_index)
{
	if (name == "base_color_texture")
	{
		texture_indices.x = texture_index;
	}
}
}        // namespace xihe::sg
