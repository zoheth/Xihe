#pragma once

#include <vector>

#include "scene_graph/script.h"
#include "scene_graph/scene.h"
#include "scene_graph/components/camera.h"


constexpr uint32_t kCascadeCount = 3;


namespace xihe::sg
{
class CascadeScript : public Script
{
public:
	CascadeScript(const std::string &name, sg::Scene &scene, sg::PerspectiveCamera &camera);

	void update(float delta_time) override;

	// std::type_index get_type() override;

	void calculate_cascade_split_depth(float lambda = 0.5) const;

	sg::OrthographicCamera &get_cascade_camera(uint32_t index) const;

	std::vector<float> &get_cascade_splits();




private:
	sg::PerspectiveCamera              &camera_;

	std::vector<std::unique_ptr<sg::OrthographicCamera>> cascade_cameras_{kCascadeCount};

	inline static uint32_t cascade_count_{kCascadeCount};

	inline static std::vector<float> cascade_splits_{};
};
}
