#include "gpu_scene.h"

#include "meshoptimizer.h"
#include <variant>

#include "rendering/subpass.h"

namespace xihe::sg
{
GpuScene::GpuScene(backend::Device &device) : device_{device}
{
}

void GpuScene::add_primitive(const MeshPrimitiveData &primitive_data)
{
	assert(primitive_data.attributes.contains("position"));

	constexpr size_t max_vertices  = 64;
	constexpr size_t max_triangles = 124;
	constexpr float  cone_weight   = 0.0f;

	const size_t max_meshlets = meshopt_buildMeshletsBound(primitive_data.index_count, max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> local_meshlets(max_meshlets);

	std::vector<uint32_t> meshlet_vertex_indices(max_meshlets * max_vertices);

	std::vector<uint8_t> meshlet_triangle(max_meshlets * max_triangles * 3);

	auto vertex_positions = reinterpret_cast<const float *>(primitive_data.attributes.at("position").data.data());

	size_t meshlet_count;

	if (primitive_data.index_type == vk::IndexType::eUint16)
	{
		// Use 16-bit indices
		auto index_data = reinterpret_cast<const uint16_t *>(primitive_data.indices.data());
		meshlet_count   = meshopt_buildMeshlets(local_meshlets.data(), meshlet_vertex_indices.data(), meshlet_triangle.data(),
		                                        index_data, primitive_data.index_count, vertex_positions, primitive_data.vertex_count,
		                                        sizeof(float) * 3,
		                                        max_vertices, max_triangles, cone_weight);
	}
	else if (primitive_data.index_type == vk::IndexType::eUint32)
	{
		// Use 32-bit indices
		auto index_data = reinterpret_cast<const uint32_t *>(primitive_data.indices.data());
		meshlet_count   = meshopt_buildMeshlets(local_meshlets.data(), meshlet_vertex_indices.data(), meshlet_triangle.data(),
		                                        index_data, primitive_data.index_count, vertex_positions, primitive_data.vertex_count,
		                                        sizeof(float) * 3,
		                                        max_vertices, max_triangles, cone_weight);
	}
	else
	{
		throw std::runtime_error("Unsupported index type. Only 16-bit and 32-bit indices are supported.");
	}

	for (const auto &[name, attrib] : primitive_data.attributes)
	{
		backend::BufferBuilder buffer_builder{attrib.data.size()};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer).with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

		backend::Buffer buffer{device_, buffer_builder};
		buffer.update(attrib.data);
		buffer.set_debug_name(fmt::format("{}: '{}' meshlet buffer", primitive_data.name, name));

		vertex_storage_buffers_.insert(std::make_pair(name, std::move(buffer)));

		VertexAttribute vertex_attrib;
		vertex_attrib.format = attrib.format;
		vertex_attrib.stride = attrib.stride;

		vertex_attributes_[name] = vertex_attrib;
	}

	for (uint32_t i = 0; i < meshlet_count; ++i)
	{
		meshopt_Meshlet &local_meshlet = local_meshlets[i];

		meshopt_Bounds bounds = meshopt_computeMeshletBounds(meshlet_vertex_indices.data() + local_meshlet.vertex_offset,
		                                                     meshlet_triangle.data() + local_meshlet.triangle_offset, local_meshlet.triangle_count, vertex_positions, primitive_data.vertex_count, sizeof(float) * 3);

		GpuMeshlet meshlet{};
		meshlet.data_offset    = meshlets_data_.size();
		meshlet.vertex_count   = local_meshlet.vertex_count;
		meshlet.triangle_count = local_meshlet.triangle_count;

		meshlet.center = glm::vec3{bounds.center[0], bounds.center[1], bounds.center[2]};
		meshlet.radius = bounds.radius;

		meshlet.cone_axis[0] = bounds.cone_axis_s8[0];
		meshlet.cone_axis[1] = bounds.cone_axis_s8[1];
		meshlet.cone_axis[2] = bounds.cone_axis_s8[2];

		meshlet.cone_cutoff = bounds.cone_cutoff_s8;
		meshlet.mesh_index  = 0;
	}
}
}        // namespace xihe::sg
