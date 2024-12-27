#pragma once

#include "clustered_lighting_pass.h"
#include "gpu_scene.h"
#include "rendering/passes/render_pass.h"
#include "scene_graph/components/light.h"

namespace xihe::rendering
{

constexpr uint32_t kMaxPerLightMeshletCount = 45000;
struct PointLight
{
	glm::vec3 position;
	float     radius;
	glm::vec3 color;
	float     intensity;
};

struct PointLightUniform
{
	PointLight point_lights[kMaxPointLightCount];
};

class PointShadowsCullingPass : public RenderPass
{
  public:
	PointShadowsCullingPass(GpuScene &gpu_scene, std::vector<sg::Light *> lights);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

	static inline uint32_t point_light_count_{0};

  private:
	GpuScene &gpu_scene_;

	PointLightUniform point_light_uniform_;
};

class PointShadowsCommandsGenerationPass : public RenderPass
{
public:
	PointShadowsCommandsGenerationPass() = default;

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

};

class PointShadowsPass : public RenderPass
{
public:
	PointShadowsPass(GpuScene &gpu_scene);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;


private:
	GpuScene &gpu_scene_;
};
}        // namespace xihe::rendering
