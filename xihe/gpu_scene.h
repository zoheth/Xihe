#pragma once

#include "backend/buffer.h"
#include "backend/device.h"
#include "scene_graph/geometry_data.h"
#include "scene_graph/scene.h"

namespace xihe
{

struct PackedVertex
{
	glm::vec4 pos;
	glm::vec4 normal;
};

struct Meshlet
{
	uint32_t vertex_offset;
	uint32_t triangle_offset;

	/* number of vertices and triangles used in the meshlet; data is stored in consecutive range defined by offset and count */
	uint32_t vertex_count;
	uint32_t triangle_count;

	glm::vec3 center;
	float     radius;
	glm::vec3 cone_axis;
	float     cone_cutoff;
};

struct MeshDraw
{
	// x = diffuse index, y = roughness index, z = normal index, w = occlusion index.
	glm::uvec4 texture_indices;
	glm::vec4  base_color_factor;
	glm::vec4  metallic_roughness_occlusion_factor;
	uint32_t   meshlet_offset;
	uint32_t   meshlet_count;
};

struct MeshData
{
	MeshData(const MeshPrimitiveData &primitive_data);

	std::vector<PackedVertex> vertices;
	std::vector<Meshlet>      meshlets;
	std::vector<uint32_t>     meshlet_vertices;
	std::vector<uint32_t>     meshlet_triangles;
	uint32_t                  meshlet_count{0};

private:
	void prepare_meshlets(const MeshPrimitiveData &primitive_data);
};

class GpuScene
{
  public:
	GpuScene() = default;

	void initialize(backend::Device &device, sg::Scene &scene);

	struct Offsets
	{
		uint32_t vertex_offset{0};
		uint32_t meshlet_offset{0};
		uint32_t meshlet_vertices_offset{0};
		uint32_t meshlet_indices_offset{0};
	};

  private:
	std::unique_ptr<backend::Buffer> global_vertex_buffer_;
	std::unique_ptr<backend::Buffer> global_meshlet_buffer_;
	std::unique_ptr<backend::Buffer> global_meshlet_vertices_buffer_;
	std::unique_ptr<backend::Buffer> global_packed_meshlet_indices_buffer_;

	std::unique_ptr<backend::Buffer> instance_buffer_;
	std::unique_ptr<backend::Buffer> mesh_draws_buffer_;
	std::unique_ptr<backend::Buffer> draw_counts_buffer_;

	std::vector<Offsets> mesh_offsets_;
};
}        // namespace xihe
