#pragma once

#include "render_pass.h"
#include "scene_graph/components/light.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/node.h"

namespace xihe::rendering
{
constexpr uint32_t kMaxLightCount = 32;

struct alignas(16) LightUniform
{
	glm::mat4 inv_view_proj;
};

struct alignas(16) Light
{
	glm::vec4 position;         // position.w represents type of light
	glm::vec4 color;            // color.w represents light intensity
	glm::vec4 direction;        // direction.w represents range
	glm::vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

struct alignas(16) DeferredLights
{
	Light directional_lights[kMaxLightCount];
	Light point_lights[kMaxLightCount];
	Light spot_lights[kMaxLightCount];
};

struct LightingState
{
	std::vector<Light> directional_lights;

	std::vector<Light> point_lights;

	std::vector<Light> spot_lights;

	backend::BufferAllocation light_buffer;
};

void bind_lighting(backend::CommandBuffer &command_buffer, const LightingState &lighting_state, uint32_t set, uint32_t binding);

class LightingPass : public RenderPass
{
  public:
	LightingPass(std::vector<sg::Light *> lights, sg::Camera &camera);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:
	void set_lighting_state(size_t light_count);

	std::vector<sg::Light *> lights_;

	sg::Camera &camera_;

	LightingState lighting_state_;

	backend::ShaderVariant   shader_variant_;
};
}        // namespace xihe::rendering