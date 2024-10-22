#pragma once

#include "rendering/subpass.h"

namespace xihe::rendering
{
class GpuSceneSubpass : public Subpass
{
  public:
	GpuSceneSubpass(rendering::RenderContext &render_context, backend::ShaderSource &&task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader);

	~GpuSceneSubpass() override = default;

	void draw(backend::CommandBuffer &command_buffer) override;
  private:
};
}        // namespace xihe::rendering
