#pragma once

#include "rendering/subpasses/geometry_subpass.h"

#include "scene_graph/scripts/cascade_script.h"

namespace xihe::rendering
{
std::unique_ptr<backend::Sampler> get_shadowmap_sampler(backend::Device &device);

class ShadowSubpass : public Subpass
{
  public:
	ShadowSubpass(RenderContext          &render_context,
	              backend::ShaderSource &&vertex_source,
	              backend::ShaderSource &&fragment_source,
	              sg::Scene              &scene,
	              sg::CascadeScript      &cascade_script,
	              uint32_t                cascade_index);

	virtual void prepare() override;

	virtual void draw(backend::CommandBuffer &command_buffer) override;


  protected:
	void update_uniforms(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index);

	void draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, const std::vector<backend::ShaderResource> &vertex_input_resources);

	void prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face) const;

	backend::PipelineLayout &prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules);

  private:
	std::vector<sg::Mesh *> meshes_;

	sg::CascadeScript &cascade_script_;

	RasterizationState base_rasterization_state_{};

	uint32_t cascade_index_{0};
};

}        // namespace xihe::rendering
