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
}        // namespace xihe::sg
