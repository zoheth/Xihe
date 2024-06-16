#pragma once

#include "rendering/rdg_pass.h"
#include "scene_graph/components/camera.h"


namespace xihe::rendering
{
class ShadowPass : public RdgPass
{
  public:
	ShadowPass(RenderContext &render_context, sg::Scene &scene, sg::Camera &camera);

	~ShadowPass();

	RenderTarget *get_render_target() const override;

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const override;

	std::vector<vk::DescriptorImageInfo> get_descriptor_image_infos(RenderTarget &render_target) const override;

  protected:
	static void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);

	static void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);

private:
	void create_render_target();

	RenderContext &render_context_;

	vk::Sampler shadowmap_sampler_;

	std::unique_ptr<RenderTarget> render_target_;
};
}        // namespace xihe::rendering
