#include "mshader_mesh.h"

#include <glm/glm.hpp>
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
backend::Buffer &MshaderMesh::get_vertex_buffer()
{
	return *vertex_buffer_;
}
backend::Buffer &MshaderMesh::get_meshlet_buffer()
{
	return *meshlet_buffer_;
}
uint32_t MshaderMesh::get_meshlet_count() const
{
	return meshlet_count_;
}
void MshaderMesh::prepare_meshlets(std::vector<Meshlet> &meshlets, const MeshPrimitiveData &primitive_data)
{
	Meshlet meshlet;
	meshlet.vertex_count = 0;
	meshlet.index_count  = 0;


	std::set<uint32_t> vertices;        // set for unique vertices
	uint32_t           triangle_check = 0;

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

	for (uint32_t i = 0; i < index_data_32.size(); i++)
	{
		// index_data is unsigned char type, casting to uint32_t* will give proper value
		meshlet.indices[meshlet.index_count] = index_data_32[i];

		if (vertices.insert(meshlet.indices[meshlet.index_count]).second)
		{
			++meshlet.vertex_count;
		}

		meshlet.index_count++;
		triangle_check = triangle_check < 3 ? ++triangle_check : 1;

		// 96 because for each traingle we draw a line in a mesh shader sample, 32 triangles/lines per meshlet = 64 vertices on output
		if (meshlet.vertex_count == 64 || meshlet.index_count == 96 || i == index_data_32.size() - 1)
		{
			if (i == index_data_32.size() - 1)
			{
				assert(triangle_check == 3);
			}

			uint32_t counter = 0;
			for (auto v : vertices)
			{
				meshlet.vertices[counter++] = v;
			}
			if (triangle_check != 3)
			{
				meshlet.index_count -= triangle_check;
				i -= triangle_check;
				triangle_check = 0;
			}

			meshlets.push_back(meshlet);
			meshlet.vertex_count = 0;
			meshlet.index_count  = 0;
			vertices.clear();
		}
	}
	meshlet_count_ = meshlets.size();
}
}        // namespace xihe::sg