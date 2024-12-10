#pragma once

#include "xihe_app.h"
#include "scene_graph/components/camera.h"

namespace xihe
{
class TempApp : public XiheApp
{
  public:
	TempApp();
	~TempApp() override = default;

	bool prepare(Window *window) override;

	void update(float delta_time) override;

	void request_gpu_features(backend::PhysicalDevice &gpu) override;

private:
	void draw_gui() override;

	xihe::sg::Camera *camera_{nullptr};

	bool show_meshlet_view_{false};
	bool freeze_frustum_{false};
	bool show_cascade_view_{false};
};
}        // namespace xihe
