#pragma once

#include "rendering/rdg_pass.h"

namespace xihe::rendering
{
class FxComputePass : public RdgPass
{
  public:
	FxComputePass(const std::string &name, RdgPassType pass_type, RenderContext &render_context);

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers) override;

  private:
	backend::ShaderSource calculate_shader_;
	backend::ShaderSource integrate_shader_;
};
}        // namespace xihe::rendering
