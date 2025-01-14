#pragma once

#include "render_pass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/light.h"
#include "scene_graph/node.h"
#include "scene_graph/scripts/cascade_script.h"

namespace xihe::rendering
{
vk::SamplerCreateInfo get_shadowmap_sampler();
vk::SamplerCreateInfo get_g_buffer_sampler();

constexpr uint32_t kMaxLightCount = 32;

const std::vector<std::string> kLightTypeDefinitions = {
    "DIRECTIONAL_LIGHT " + std::to_string(static_cast<float>(sg::LightType::kDirectional)),
    "POINT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::kPoint)),
    "SPOT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::kSpot))};

struct alignas(16) LightUniform
{
	glm::mat4 inv_view_proj;
	glm::vec3 camera_position;
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

struct alignas(16) ShadowUniform
{
	float                                cascade_split_depth[4];             // Split depths in view space
	std::array<glm::mat4, kCascadeCount> shadowmap_projection_matrix;        // Projection matrix used to render shadowmap
};

void bind_lighting(backend::CommandBuffer &command_buffer, const LightingState &lighting_state, uint32_t set, uint32_t binding);

class LightingPass : public RenderPass
{
  public:
	LightingPass(std::vector<sg::Light *> lights, sg::Camera &camera, sg::CascadeScript *cascade_script = nullptr, Texture *irradiance = nullptr, Texture *prefiltered = nullptr, Texture *brdf_lut = nullptr);


	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

	static void show_cascade_view(bool show);

  protected:
	void set_lighting_state(size_t light_count);

	void set_pipeline_state(backend::CommandBuffer &command_buffer);

	std::vector<sg::Light *> lights_;

	sg::Camera &camera_;

	LightingState lighting_state_;

	backend::ShaderVariant shader_variant_;

	backend::ShaderVariant shader_variant_cascade_;

	sg::CascadeScript *cascade_script_;

	Texture *irradiance_{nullptr};
	Texture *prefiltered_{nullptr};
	Texture *brdf_lut_{nullptr};

	inline static bool show_cascade_view_{false};
};
}        // namespace xihe::rendering