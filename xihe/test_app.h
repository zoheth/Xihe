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


private:

	xihe::sg::Camera *camera_{nullptr};

	std::unique_ptr<backend::Sampler> shadowmap_sampler_{nullptr};

	std::unique_ptr<rendering::RenderTarget> shadow_render_target_{nullptr};

	std::unique_ptr<rendering::RenderPipeline> shadow_render_pipeline_{nullptr};

};
}        // namespace xihe
