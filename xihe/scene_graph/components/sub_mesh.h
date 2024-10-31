#pragma once

#include "backend/buffer.h"
#include "backend/shader_module.h"
#include "scene_graph/geometry_data.h"
#include "scene_graph/component.h"

namespace xihe::sg
{
class Material;

class SubMesh : public Component
{
  public:
	// SubMesh(const std::string &name = {});

	SubMesh(const MeshPrimitiveData &primitive_data, backend::Device &device);

	virtual ~SubMesh() = default;

	virtual std::type_index get_type() override;

	vk::IndexType index_type{};

	std::uint32_t index_offset = 0;

	std::uint32_t vertex_count = 0;

	std::uint32_t index_count = 0;

	std::unordered_map<std::string, backend::Buffer> vertex_buffers;

	std::unique_ptr<backend::Buffer> index_buffer;

	void set_attribute(const std::string &name, const VertexAttribute &attribute);

	bool get_attribute(const std::string &name, VertexAttribute &attribute) const;

	void set_material(const Material &material);

	const Material *get_material() const;

	const backend::ShaderVariant &get_shader_variant() const;

	backend::ShaderVariant &get_mut_shader_variant();

  private:
	std::unordered_map<std::string, VertexAttribute> vertex_attributes_;

	const Material *material_{nullptr};

	backend::ShaderVariant shader_variant_;

	void compute_shader_variant();
};
}
