#pragma once

#include "rendering/rdg_pass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/scripts/cascade_script.h"

namespace xihe::rendering
{
class MainPass : public RdgPass
{
public:
	MainPass(const std::string &name, RdgPassType pass_type, RenderContext &render_context, sg::Scene &scene, sg::Camera &camera, sg::CascadeScript *cascade_script_ptr);

	std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image) const override;

	void prepare(backend::CommandBuffer &command_buffer) override;

  protected:
	void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents) override;
	void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target) override;
};
}
