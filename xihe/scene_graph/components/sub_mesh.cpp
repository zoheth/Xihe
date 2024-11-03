#include "sub_mesh.h"

#include "material.h"
#include "rendering/subpass.h"

namespace xihe::sg
{

//SubMesh::SubMesh(const std::string &name) :
//    Component{name}
//{}

SubMesh::SubMesh(const MeshPrimitiveData &primitive_data, backend::Device &device) :
	Component{primitive_data.name}
{
	vertex_count = primitive_data.vertex_count;

	for (const auto &[name, attrib] : primitive_data.attributes)
	{
		backend::BufferBuilder buffer_builder{attrib.data.size()};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eVertexBuffer).with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

		backend::Buffer buffer{device, buffer_builder};
		buffer.update(attrib.data);
		buffer.set_debug_name(fmt::format("{}: '{}' vertex buffer", primitive_data.name, name));

		vertex_buffers.insert(std::make_pair(name, std::move(buffer)));

		VertexAttribute vertex_attrib;
		vertex_attrib.format = attrib.format;
		vertex_attrib.stride = attrib.stride;

		set_attribute(name, vertex_attrib);
	}

	if (!primitive_data.indices.empty())
	{
		backend::BufferBuilder buffer_builder{primitive_data.indices.size()};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eIndexBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

		index_buffer = std::make_unique<backend::Buffer>(device, buffer_builder);
		index_buffer->set_debug_name(fmt::format("{}: index buffer", primitive_data.name));

		index_buffer->update(primitive_data.indices);

		index_type  = primitive_data.index_type;
		index_count = primitive_data.index_count;
	}
}

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
