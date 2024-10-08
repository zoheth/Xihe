#pragma once

#include "xihe_app.h"

#include "scene_graph/components/camera.h"

namespace xihe
{

class MeshShadingApp : public XiheApp
{
  public:
	MeshShadingApp();
	~MeshShadingApp() override = default;

	bool prepare(Window *window) override;

	void update(float delta_time) override;

	void request_gpu_features(backend::PhysicalDevice &gpu) override;

  private:
	xihe::sg::Camera *camera_{nullptr};
};
}        // namespace xihe
