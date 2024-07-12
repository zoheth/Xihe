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

	std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image) const override;

	std::vector<vk::DescriptorImageInfo> get_descriptor_image_infos(RenderTarget &render_target) const override;

	vk::Sampler get_shadowmap_sampler() const;

  protected:
	void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents) override;

	void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target) override;

private:
	void create_owned_render_target();

	vk::Sampler shadowmap_sampler_;

	bool is_first_frame_ = true;
};
}        // namespace xihe::rendering
