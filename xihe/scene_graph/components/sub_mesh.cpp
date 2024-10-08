#include "sub_mesh.h"

#include "material.h"
#include "rendering/subpass.h"

#include "meshoptimizer.h"

namespace xihe::sg
{

template <typename T>
void generate_meshlets(const SubMesh &sub_mesh, std::vector<float> &vertex_positions, std::vector<T> &indices)
{
	constexpr size_t max_vertices  = 64;
	constexpr size_t max_triangles = 124;
	constexpr float  cone_weight   = 0.0f;

	const size_t max_meshlets = meshopt_buildMeshletsBound(sub_mesh.index_count, max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> local_meshlets(max_meshlets);

	std::vector<uint32_t> meshlet_vertex_indices(max_meshlets * max_vertices);

	std::vector<uint8_t> meshlet_triangle(max_meshlets * max_triangles * 3);

	size_t meshlet_count = meshopt_buildMeshlets(local_meshlets.data(), meshlet_vertex_indices.data(), meshlet_triangle.data(),
	                                             indices.data(), sub_mesh.index_count, vertex_positions.data(), sub_mesh.vertex_count,
	                                             sizeof(float) * 3,
	                                             max_vertices, max_triangles, cone_weight);
}

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
}        // namespace xihe::sg
