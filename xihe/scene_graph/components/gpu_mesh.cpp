#include "gpu_mesh.h"

#include "meshoptimizer.h"

#include <variant>

namespace xihe::sg
{
GpuMesh::GpuMesh(const MeshPrimitiveData &primitive_data, backend::Device &device)
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
		meshlet_count = meshopt_buildMeshlets(local_meshlets.data(), meshlet_vertex_indices.data(), meshlet_triangle.data(),
		                                             index_data, primitive_data.index_count, vertex_positions, primitive_data.vertex_count,
		                                             sizeof(float) * 3,
		                                             max_vertices, max_triangles, cone_weight);
	}
	else if (primitive_data.index_type == vk::IndexType::eUint32)
	{
		// Use 32-bit indices
		auto index_data = reinterpret_cast<const uint32_t *>(primitive_data.indices.data());
		meshlet_count = meshopt_buildMeshlets(local_meshlets.data(), meshlet_vertex_indices.data(), meshlet_triangle.data(),
		                                             index_data, primitive_data.index_count, vertex_positions, primitive_data.vertex_count,
		                                             sizeof(float) * 3,
		                                             max_vertices, max_triangles, cone_weight);
	}
	else
	{
		throw std::runtime_error("Unsupported index type. Only 16-bit and 32-bit indices are supported.");
	}


}
}
