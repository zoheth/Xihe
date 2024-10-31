#pragma once

#include "backend/buffer.h"
#include "backend/shader_module.h"
#include "scene_graph/component.h"
#include "scene_graph/geometry_data.h"

namespace xihe::sg
{
class Material;

struct alignas(16) GpuMeshlet
{
	glm::vec3 center;
	glm::f32  radius;

	int8_t cone_axis[3];
	int8_t cone_cutoff;

	uint32_t data_offset;
	uint32_t mesh_index;
	uint8_t  vertex_count;
	uint8_t  triangle_count;
};

struct MeshletToMeshIndex
{
	uint32_t mesh_index;
	uint32_t primitive_index;
};

// struct GpuMeshletVertexPosition
//{
//	float position[3];
//	float padding;
// };
//
// struct GpuMeshletVertexData
//{
//	uint8_t  normal[4];
//	uint8_t  tangent[4];
//	uint16_t uv_coords[2];
//	float    padding;
// };

struct MeshInfo
{
	const Material *material_{nullptr};

	backend::ShaderVariant shader_variant_;

	uint32_t meshlet_offset{0};
	uint32_t meshlet_count{0};
};

class GpuScene
{
  public:
	GpuScene(backend::Device &device);

	void add_primitive(const MeshPrimitiveData &primitive_data);

  private:
	backend::Device                                 &device_;

	std::unordered_map<std::string, backend::Buffer> vertex_storage_buffers_;
	std::unordered_map<std::string, VertexAttribute> vertex_attributes_;



	// Meshlet data
	std::vector<uint32_t> meshlets_data_;
};
}        // namespace xihe::sg
