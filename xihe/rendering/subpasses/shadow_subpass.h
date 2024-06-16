#pragma once

#include "rendering/subpasses/geometry_subpass.h"

#include "scene_graph/components/camera.h"

constexpr uint32_t kCascadeCount = 3;

namespace xihe::rendering
{

struct alignas(16) ShadowUniform
{
	float                                cascade_split_depth[4];             // Split depths in view space
	std::array<glm::mat4, kCascadeCount> shadowmap_projection_matrix;        // Projection matrix used to render shadowmap
};



class ShadowSubpass : public Subpass
{
  public:
	ShadowSubpass(RenderContext          &render_context,
	              backend::ShaderSource &&vertex_source,
	              backend::ShaderSource &&fragment_source,
	              sg::Scene              &scene,
	              sg::PerspectiveCamera  &camera,
	              uint32_t                cascade_index);

	virtual void prepare() override;

	virtual void draw(backend::CommandBuffer &command_buffer) override;

	void calculate_cascade_split_depth(float lambda = 0.5);

	static ShadowUniform &get_shadow_uniform() { return shadow_uniform_; }

  protected:
	void update_uniforms(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index);

	void draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, const std::vector<backend::ShaderResource> &vertex_input_resources);

	void prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face) const;

	backend::PipelineLayout &prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules);

  private:
	sg::PerspectiveCamera &camera_;

	std::vector<sg::Mesh *> meshes_;

	sg::Scene &scene_;

	uint32_t thread_index_{0};

	RasterizationState base_rasterization_state_{};

	inline static uint32_t cascade_count_{kCascadeCount};

	uint32_t cascade_index_{0};

	std::unique_ptr<sg::OrthographicCamera> cascade_camera_;

	inline static std::vector<float> cascade_splits_{};

	inline static ShadowUniform shadow_uniform_{};
};
}        // namespace xihe::rendering
