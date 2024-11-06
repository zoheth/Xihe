#include "mshader_mesh.h"

#include <glm/glm.hpp>
#include "meshoptimizer.h"

#include "rendering/subpass.h"
#include "scene_graph/components/material.h"

namespace
{
glm::vec4 convert_to_vec4(const std::vector<uint8_t> &data, uint32_t offset, float padding = 1.0f)
{
	if (data.size() < offset + 3 * sizeof(float))
		throw std::runtime_error("Data size is too small for conversion to vec4.");

	float x, y, z;
	std::memcpy(&x, &data[offset], sizeof(float));
	std::memcpy(&y, &data[offset + sizeof(float)], sizeof(float));
	std::memcpy(&z, &data[offset + 2 * sizeof(float)], sizeof(float));

	return glm::vec4(x, y, z, padding);
}
}
namespace xihe::sg
{
MshaderMesh::MshaderMesh(const MeshPrimitiveData &primitive_data, backend::Device &device)
{
	auto pos_it    = primitive_data.attributes.find("position");
	auto normal_it = primitive_data.attributes.find("normal");
	auto uv_it     = primitive_data.attributes.find("texcoord_0");

	if (pos_it == primitive_data.attributes.end() || normal_it == primitive_data.attributes.end() || uv_it == primitive_data.attributes.end())
	{
		throw std::runtime_error("Position, Normal or UV attribute not found.");
	}

	const VertexAttributeData &pos_attr    = pos_it->second;
	const VertexAttributeData &normal_attr = normal_it->second;
	const VertexAttributeData &uv_attr     = uv_it->second;

	if (pos_attr.stride == 0 || normal_attr.stride == 0)
	{
		throw std::runtime_error("Stride for position or normal attribute is zero.");
	}
	uint32_t vertex_count = primitive_data.vertex_count;
	std::vector<PackedVertex> packed_vertex_data;
	packed_vertex_data.reserve(vertex_count);

	for (size_t i = 0; i < vertex_count; i++)
	{
		uint32_t  pos_offset    = i * pos_attr.stride;
		uint32_t  normal_offset = i * normal_attr.stride;
		uint32_t uv_offset     = i * uv_attr.stride;
		float     u, v;
		std::memcpy(&u, &uv_attr.data[uv_offset], sizeof(float));
		std::memcpy(&v, &uv_attr.data[uv_offset + sizeof(float)], sizeof(float));
		glm::vec4 pos           = convert_to_vec4(pos_attr.data, pos_offset, u);
		glm::vec4 normal        = convert_to_vec4(normal_attr.data, normal_offset, v);
		packed_vertex_data.push_back({pos, normal});
	}
	{
		backend::BufferBuilder buffer_builder{packed_vertex_data.size() * sizeof(PackedVertex)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

		vertex_data_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		vertex_data_buffer_->set_debug_name(fmt::format("{}: vertex buffer", primitive_data.name));
		vertex_data_buffer_->update(packed_vertex_data);
	}

	std::vector<Meshlet> meshlets;
	prepare_meshlets(meshlets, primitive_data, device);

	{
		backend::BufferBuilder buffer_builder{meshlets.size() * sizeof(Meshlet)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

		meshlet_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		meshlet_buffer_->set_debug_name(fmt::format("{}: meshlet buffer", primitive_data.name));

		meshlet_buffer_->update(meshlets);
	}

}
std::type_index MshaderMesh::get_type()
{
	return typeid(MshaderMesh);
}
backend::Buffer &MshaderMesh::get_vertex_buffer() const
{
	return *vertex_data_buffer_;
}
backend::Buffer &MshaderMesh::get_meshlet_buffer() const
{
	return *meshlet_buffer_;
}

backend::Buffer & MshaderMesh::get_meshlet_vertices_buffer() const
{
	return *meshlet_vertices_buffer_;
}

backend::Buffer & MshaderMesh::get_packed_meshlet_indices_buffer() const
{
	return *packed_meshlet_indices_buffer_;
}

void MshaderMesh::set_material(const Material &material)
{
	material_ = &material;
	compute_shader_variant();
}

const Material * MshaderMesh::get_material() const
{
	return material_;
}

uint32_t MshaderMesh::get_meshlet_count() const
{
	return meshlet_count_;
}

const backend::ShaderVariant & MshaderMesh::get_shader_variant() const
{
	return shader_variant_;
}

backend::ShaderVariant & MshaderMesh::get_mut_shader_variant()
{
	return shader_variant_;
}

void MshaderMesh::compute_shader_variant()
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
}

void MshaderMesh::prepare_meshlets(std::vector<Meshlet> &meshlets, const MeshPrimitiveData &primitive_data, backend::Device &device)
{
	std::vector<uint32_t> index_data_32;
	if (primitive_data.index_type == vk::IndexType::eUint16)
	{
		const uint16_t *index_data_16 = reinterpret_cast<const uint16_t *>(primitive_data.indices.data());
		index_data_32.resize(primitive_data.index_count);
		for (size_t i = 0; i < primitive_data.index_count; ++i)
		{
			index_data_32[i] = static_cast<uint32_t>(index_data_16[i]);
		}
	}
	else if (primitive_data.index_type == vk::IndexType::eUint32)
	{
		index_data_32.assign(
            reinterpret_cast<const uint32_t *>(primitive_data.indices.data()),
            reinterpret_cast<const uint32_t *>(primitive_data.indices.data()) + primitive_data.index_count);
	}

	// Use meshoptimizer to generate meshlets
	constexpr size_t max_vertices  = 64;
	constexpr size_t max_triangles = 124;
	constexpr float  cone_weight   = 0.0f;

	const size_t max_meshlets = meshopt_buildMeshletsBound(index_data_32.size(), max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> local_meshlets(max_meshlets);
	std::vector<uint32_t>        meshlet_vertex_indices(max_meshlets * max_vertices);
	std::vector<uint8_t>         meshlet_triangle_indices(max_meshlets * max_triangles * 3);

	auto vertex_positions = reinterpret_cast<const float *>(primitive_data.attributes.at("position").data.data());

	size_t meshlet_count = meshopt_buildMeshlets(
	    local_meshlets.data(),
	    meshlet_vertex_indices.data(),
	    meshlet_triangle_indices.data(),
	    index_data_32.data(),
	    index_data_32.size(),
	    vertex_positions,
	    primitive_data.vertex_count,
	    sizeof(float) * 3,
	    max_vertices,
	    max_triangles,
	    cone_weight);

	local_meshlets.resize(meshlet_count);

	std::vector<uint32_t> meshlet_vertices;
	std::vector<uint32_t> meshlet_triangles;

	// Convert meshopt_Meshlet to our Meshlet structure
	for (size_t i = 0; i < meshlet_count; ++i)
	{
		const meshopt_Meshlet &local_meshlet = local_meshlets[i];

		Meshlet meshlet;
		meshlet.vertex_offset   = static_cast<uint32_t>(meshlet_vertices.size());
		meshlet.triangle_offset = static_cast<uint32_t>(meshlet_triangles.size());
		meshlet.vertex_count    = static_cast<uint32_t>(local_meshlet.vertex_count);
		meshlet.triangle_count  = static_cast<uint32_t>(local_meshlet.triangle_count);

		for (size_t j = 0; j < local_meshlet.vertex_count; ++j)
		{
			uint32_t vertex_index = meshlet_vertex_indices[local_meshlet.vertex_offset + j];
			meshlet_vertices.push_back(vertex_index);
		}

		size_t triangle_count = local_meshlet.triangle_count;
		size_t triangle_base  = local_meshlet.triangle_offset;

		for (size_t j = 0; j < triangle_count; ++j)
		{
			uint8_t idx0 = meshlet_triangle_indices[triangle_base + j * 3 + 0];
			uint8_t idx1 = meshlet_triangle_indices[triangle_base + j * 3 + 1];
			uint8_t idx2 = meshlet_triangle_indices[triangle_base + j * 3 + 2];

			// Pack three uint8_t indices into one uint32_t
			uint32_t packed_triangle = idx0 | (idx1 << 8) | (idx2 << 16);
			meshlet_triangles.push_back(packed_triangle);
		}

		meshlets.push_back(meshlet);
	}

	{
		backend::BufferBuilder buffer_builder{meshlet_vertices.size() * 4};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		meshlet_vertices_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		meshlet_vertices_buffer_->update(meshlet_vertices);
	}

	{
		backend::BufferBuilder buffer_builder{meshlet_triangles.size() * 4};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		packed_meshlet_indices_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		packed_meshlet_indices_buffer_->update(meshlet_triangles);
	}

	meshlet_count_ = meshlets.size();
}
}        // namespace xihe::sg