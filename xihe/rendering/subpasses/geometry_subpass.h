#pragma once

#include "rendering/subpass.h"

namespace xihe
{

namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}

namespace rendering
{
class GeometrySubpass : public Subpass
{
  public:
	GeometrySubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera);

	~GeometrySubpass() override = default;

	void prepare() override;

	void draw(backend::CommandBuffer &command_buffer) override;

  protected:
	virtual void update_uniform(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index);

	void draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, vk::FrontFace front_face = vk::FrontFace::eCounterClockwise);

	virtual void prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face, bool double_sided_material);

	virtual backend::PipelineLayout &prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules);

	virtual void prepare_push_constants(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

	virtual void draw_submesh_command(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

	/**
	 * @brief Sorts objects based on distance from camera and classifies them
	 *        into opaque and transparent in the arrays provided
	 */
	void get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &opaque_nodes,
	                      std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &transparent_nodes) const;

	sg::Camera &camera_;

	std::vector<sg::Mesh *> meshes_;

	sg::Scene &scene_;

	RasterizationState base_rasterization_state_{};
};
}
}
