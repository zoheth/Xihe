#include "sub_mesh.h"

#include "material.h"
#include "rendering/subpass.h"

#include "meshoptimizer.h"

namespace xihe::sg
{
SubMesh::SubMesh(const std::string &name) :
    Component{name}
{}

std::type_index SubMesh::get_type()
{
	return typeid(SubMesh);
}

void SubMesh::set_attribute(const std::string &attribute_name, const VertexAttribute &attribute)
{
	vertex_attributes_[attribute_name] = attribute;

	compute_shader_variant();
}

bool SubMesh::get_attribute(const std::string &attribute_name, VertexAttribute &attribute) const
{
	auto attrib_it = vertex_attributes_.find(attribute_name);

	if (attrib_it == vertex_attributes_.end())
	{
		return false;
	}

	attribute = attrib_it->second;

	return true;
}

void SubMesh::set_material(const Material &new_material)
{
	material_ = &new_material;

	compute_shader_variant();
}

const Material *SubMesh::get_material() const
{
	return material_;
}

const backend::ShaderVariant &SubMesh::get_shader_variant() const
{
	return shader_variant_;
}

void SubMesh::compute_shader_variant()
{
	shader_variant_.clear();

	if (material_ != nullptr)
	{
		for (auto &texture : material_->textures)
		{
			std::string tex_name = texture.first;
			std::ranges::transform(tex_name, tex_name.begin(), ::toupper);

			shader_variant_.add_define("HAS_" + tex_name);
		}
	}

	for (auto &attribute : vertex_attributes_)
	{
		std::string attrib_name = attribute.first;
		std::ranges::transform(attrib_name, attrib_name.begin(), ::toupper);
		shader_variant_.add_define("HAS_" + attrib_name);
	}
}

backend::ShaderVariant &SubMesh::get_mut_shader_variant()
{
	return shader_variant_;
}
}
