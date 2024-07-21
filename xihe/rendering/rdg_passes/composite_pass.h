#pragma once

#include "rendering/rdg_pass.h"

namespace xihe::rendering
{
class CompositePass : public RdgPass
{
public:
	CompositePass(std::string name, RenderContext &render_context, RdgPassType pass_type);

	void prepare(backend::CommandBuffer &command_buffer) override;

	std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image) const override;


private:
	void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target) override;
	std::unique_ptr<backend::Sampler> sampler_;
};
}
