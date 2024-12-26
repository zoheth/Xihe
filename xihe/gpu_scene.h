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

	uint32_t mesh_draw_index;
	glm::uvec3 padding;
};

struct MeshDraw
{
	// x = diffuse index, y = roughness index, z = normal index, w = occlusion index.
	glm::uvec4 texture_indices;
	glm::vec4  base_color_factor;
	glm::vec4  metallic_roughness_occlusion_factor;
	uint32_t   meshlet_offset;
	uint32_t   meshlet_count;
	// Global offset into the vertex buffer for all meshlets in this mesh.
	// Individual meshlet vertex offsets are stored in their respective Meshlet structs.
	uint32_t mesh_vertex_offset;
	// Global offset into the triangle buffer for all meshlets in this mesh.
	// Individual meshlet triangle offsets are stored in their respective Meshlet structs.
	uint32_t mesh_triangle_offset;
};

struct MeshInstanceDraw
{
	glm::mat4 model;
	glm::mat4 model_inverse;
	uint32_t  mesh_draw_id;
	uint32_t  padding[3];
};

// This structure is only used to calculate size, with the specific values written by the GPU
struct MeshDrawCommand
{
	// VkDrawMeshTasksIndirectCommandEXT
	uint32_t group_count_x;
	uint32_t group_count_y;
	uint32_t group_count_z;
	uint32_t instance_index;
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

	backend::Buffer &get_instance_buffer() const;
	backend::Buffer &get_mesh_draws_buffer() const;
	backend::Buffer &get_draw_command_buffer() const;
	backend::Buffer &get_draw_counts_buffer() const;

	backend::Buffer &get_global_vertex_buffer() const;
	backend::Buffer &get_global_meshlet_buffer() const;
	backend::Buffer &get_global_meshlet_vertices_buffer() const;
	backend::Buffer &get_global_packed_meshlet_indices_buffer() const;

	uint32_t get_instance_count() const;

  private:
	uint32_t instance_count_{};

	std::unique_ptr<backend::Buffer> global_vertex_buffer_;
	std::unique_ptr<backend::Buffer> global_meshlet_buffer_;
	std::unique_ptr<backend::Buffer> global_meshlet_vertices_buffer_;
	std::unique_ptr<backend::Buffer> global_packed_meshlet_indices_buffer_;

	std::unique_ptr<backend::Buffer> instance_buffer_;
	std::unique_ptr<backend::Buffer> mesh_draws_buffer_;
	std::unique_ptr<backend::Buffer> draw_command_buffer_;
	std::unique_ptr<backend::Buffer> draw_counts_buffer_;
};
}        // namespace xihe
