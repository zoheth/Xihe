#include "mshader_mesh.h"

#include <glm/glm.hpp>
#include "meshoptimizer.h"

#include "rendering/subpass.h"

namespace
{
glm::vec4 convert_to_vec4(const std::vector<uint8_t> &data, uint32_t offset)
{
	if (data.size() < offset + 3 * sizeof(float))
		throw std::runtime_error("Data size is too small for conversion to vec4.");

	float x, y, z;
	std::memcpy(&x, &data[offset], sizeof(float));
	std::memcpy(&y, &data[offset + sizeof(float)], sizeof(float));
	std::memcpy(&z, &data[offset + 2 * sizeof(float)], sizeof(float));

	return glm::vec4(x, y, z, 1.0f);        // Set w to 1.0f for position
}
}
namespace xihe::sg
{
MshaderMesh::MshaderMesh(const MeshPrimitiveData &primitive_data, backend::Device &device)
{
	std::vector<AlignedVertex> aligned_vertex_data;

	auto pos_it    = primitive_data.attributes.find("position");
	auto normal_it = primitive_data.attributes.find("normal");

	if (pos_it == primitive_data.attributes.end() || normal_it == primitive_data.attributes.end())
	{
		throw std::runtime_error("Position or Normal attribute not found.");
	}

	const VertexAttributeData &pos_attr    = pos_it->second;
	const VertexAttributeData &normal_attr = normal_it->second;

	if (pos_attr.stride == 0 || normal_attr.stride == 0)
	{
		throw std::runtime_error("Stride for position or normal attribute is zero.");
	}
	uint32_t vertex_count = primitive_data.vertex_count;
	aligned_vertex_data.reserve(vertex_count);

	for (size_t i = 0; i < vertex_count; i++)
	{
		uint32_t  pos_offset    = i * pos_attr.stride;
		uint32_t  normal_offset = i * normal_attr.stride;
		glm::vec4 pos           = convert_to_vec4(pos_attr.data, pos_offset);
		glm::vec4 normal        = convert_to_vec4(normal_attr.data, normal_offset);
		aligned_vertex_data.push_back({pos, normal});
	}
	{
		backend::BufferBuilder buffer_builder{aligned_vertex_data.size() * sizeof(AlignedVertex)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

		vertex_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		vertex_buffer_->set_debug_name(fmt::format("{}: vertex buffer", primitive_data.name));
		vertex_buffer_->update(aligned_vertex_data);
	}

	std::vector<Meshlet> meshlets;
	prepare_meshlets(meshlets, primitive_data);

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
	return *vertex_buffer_;
}
backend::Buffer &MshaderMesh::get_meshlet_buffer() const
{
	return *meshlet_buffer_;
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

void MshaderMesh::prepare_meshlets(std::vector<Meshlet> &meshlets, const MeshPrimitiveData &primitive_data)
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
		index_data_32 = std::vector<uint32_t>(reinterpret_cast<const uint32_t *>(primitive_data.indices.data()),
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

	// Convert meshopt_Meshlet to our Meshlet structure
	for (size_t i = 0; i < meshlet_count; ++i)
	{
		const meshopt_Meshlet &local_meshlet = local_meshlets[i];

		Meshlet meshlet;
		meshlet.vertex_count = static_cast<uint32_t>(local_meshlet.vertex_count);
		meshlet.index_count  = static_cast<uint32_t>(local_meshlet.triangle_count * 3);        // 3 indices per triangle

		// Copy vertices
		for (size_t j = 0; j < local_meshlet.vertex_count; ++j)
		{
			meshlet.vertices[j] = meshlet_vertex_indices[local_meshlet.vertex_offset + j];
		}

		// Copy indices
		for (size_t j = 0; j < meshlet.index_count; ++j)
		{
			meshlet.indices[j] = meshlet_triangle_indices[local_meshlet.triangle_offset + j];
		}

		meshlets.push_back(meshlet);
	}

	meshlet_count_ = meshlets.size();
}
}        // namespace xihe::sg