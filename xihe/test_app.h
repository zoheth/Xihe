#pragma once

#include "xihe_app.h"

#include "rendering/subpass.h"
#include "scene_graph/components/camera.h"

namespace xihe
{

class TestApp : public XiheApp
{
  public:
	TestApp()           = default;
	~TestApp() override = default;

	bool prepare(Window *window) override;

	void update(float delta_time) override;

protected:
	std::unique_ptr<rendering::RenderTarget> create_render_target(backend::Image &&swapchain_image) override;

	void create_shadowmap_render_target();

	void draw_renderpass(backend::CommandBuffer &command_buffer, rendering::RenderTarget &render_target) override;

private:

	std::unique_ptr<rendering::RenderPipeline> create_shadow_render_pipeline();

	xihe::sg::Camera *camera_{nullptr};

	std::unique_ptr<rendering::RenderTarget> shadow_render_target_{nullptr};

	std::unique_ptr<rendering::RenderPipeline> shadow_render_pipeline_{nullptr};

};
}        // namespace xihe
