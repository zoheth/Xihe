#pragma once

#include "xihe_app.h"

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
};
}        // namespace xihe
