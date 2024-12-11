#pragma once

#include "render_pass.h"

#include "scene_graph/components/camera.h"
#include "scene_graph/components/light.h"
#include "scene_graph/scripts/cascade_script.h"

namespace xihe::rendering
{

struct SortedLight
{
	uint32_t light_index;
	float    projected_z;
	float    projected_min;
	float    projected_max;
};

class ClusteredLightingPass : public RenderPass
{
  public:
	ClusteredLightingPass(std::vector<sg::Light *> lights, sg::Camera &camera, sg::CascadeScript *cascade_script = nullptr);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:
	void sort_lights();

	sg::PerspectiveCamera &camera_;

	std::vector<sg::Light *> lights_;
	std::vector<SortedLight> sorted_lights_;
};
}        // namespace xihe::rendering
