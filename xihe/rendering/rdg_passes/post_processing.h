#pragma once

#include "rendering/rdg_pass.h"

namespace xihe::rendering
{
class BlurPass : public RdgPass
{
  public:
	BlurPass(std::string name, const RdgPassType pass_type, RenderContext &render_context);

	std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image) const override;

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers) override;

  private:
	backend::PipelineLayout *threshold_pipeline_layout_;
	backend::PipelineLayout *blur_down_pipeline_layout_;
	backend::PipelineLayout *blur_up_pipeline_layout_;

	std::unique_ptr<backend::Sampler> linear_sampler_;

};
}        // namespace xihe::rendering
