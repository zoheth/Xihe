#pragma once

#include "xihe_app.h"

#include "rendering/subpass.h"
#include "scene_graph/components/camera.h"

namespace xihe
{

class TestApp : public XiheApp
{
  public:
	TestApp();
	~TestApp() override = default;

	bool prepare(Window *window) override;

	void update(float delta_time) override;

	void request_gpu_features(backend::PhysicalDevice &gpu) override;

private:
	void draw_gui() override;

	xihe::sg::Camera *camera_{nullptr};

	std::unique_ptr<backend::Sampler> shadowmap_sampler_{nullptr};

	std::unique_ptr<backend::Sampler> linear_sampler_{nullptr};

	std::unique_ptr<rendering::RenderTarget> shadow_render_target_{nullptr};

	std::unique_ptr<rendering::RenderPipeline> shadow_render_pipeline_{nullptr};

	bool show_meshlet_view_{false};
	bool freeze_frustum_{false};
	bool show_cascade_view_{false};

};
}        // namespace xihe
