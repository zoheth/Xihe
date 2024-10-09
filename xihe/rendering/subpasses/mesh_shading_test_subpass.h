#pragma once

#include "rendering/subpass.h"

namespace xihe::rendering
{
class MeshShadingTestSubpass : public rendering::Subpass
{
  public:
	struct CullUniform
	{
		float cull_center_x   = -1.0f;
		float cull_center_y   = -1.0f;
		float cull_radius     = 1.0f;
		float meshlet_density = 2.0f;
	} cull_uniform_{};

	MeshShadingTestSubpass(rendering::RenderContext &render_context, backend::ShaderSource &&task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader);

	void prepare() override;

	void draw(backend::CommandBuffer &command_buffer) override;

	void update_uniform(backend::CommandBuffer &command_buffer, size_t thread_index) const;

  private:
	int32_t density_level_ = 2;
};
}        // namespace xihe::rendering