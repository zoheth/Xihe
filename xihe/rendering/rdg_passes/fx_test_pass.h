#pragma once

#include "rendering/rdg_pass.h"

constexpr uint32_t kParticleCount = 4 * 16384;

namespace xihe::rendering
{
class FxComputePass : public RdgPass
{
  public:
	FxComputePass(const std::string &name, RenderContext &render_context, RdgPassType pass_type);

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers) override;

  private:
	backend::ShaderSource calculate_shader_;
	backend::ShaderSource integrate_shader_;
};
}        // namespace xihe::rendering
