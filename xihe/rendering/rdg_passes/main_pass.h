#pragma once

#include "rendering/rdg_pass.h"

namespace xihe::rendering
{
class MainPass : public RdgPass
{
public:
	MainPass(RenderContext &render_context, sg::Scene &scene);

	std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image) override;

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const override;
  protected:
	static void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);

	static void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);
};
}
