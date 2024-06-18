#pragma once

#include "rendering/rdg_pass.h"
#include "scene_graph/components/camera.h"


namespace xihe::rendering
{
class ShadowPass : public RdgPass
{
  public:
	ShadowPass(const std::string &name, RenderContext &render_context, sg::Scene &scene, sg::Camera &camera);

	~ShadowPass() override;

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const override;

	std::vector<vk::DescriptorImageInfo> get_descriptor_image_infos(RenderTarget &render_target) const override;

  protected:
	static void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);

	static void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);

private:
	void create_owned_render_target();

	vk::Sampler shadowmap_sampler_;
};
}        // namespace xihe::rendering
