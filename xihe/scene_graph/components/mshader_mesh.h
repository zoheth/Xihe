#pragma once

#include "backend/buffer.h"
#include "backend/shader_module.h"
#include "scene_graph/component.h"
#include "scene_graph/geometry_data.h"

namespace xihe::sg
{
class Material;

struct AlignedVertex
{
	glm::vec4 pos;
	glm::vec4 normal;
};

struct Meshlet
{
	uint32_t vertices[64];
	uint32_t indices[126];
	uint32_t vertex_count;
	uint32_t index_count;
};

class MshaderMesh : public Component
{
  public:

	MshaderMesh(const MeshPrimitiveData &primitive_data, backend::Device &device);

	virtual ~MshaderMesh() = default;

	virtual std::type_index get_type() override;

	backend::Buffer &get_vertex_buffer();
	backend::Buffer &get_meshlet_buffer();

	uint32_t get_meshlet_count() const;

  private:
	void prepare_meshlets(std::vector<Meshlet> &meshlets, const MeshPrimitiveData &primitive_data);

	uint32_t meshlet_count_{0};

	std::unique_ptr<backend::Buffer> vertex_buffer_;
	std::unique_ptr<backend::Buffer> meshlet_buffer_;
};
}        // namespace xihe::sg
