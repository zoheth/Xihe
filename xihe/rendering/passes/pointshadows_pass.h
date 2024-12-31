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

class PointShadowsResources
{
  public:
	static PointShadowsResources &get();

	static void destroy();

	void initialize(backend::Device &device, std::vector<sg::Light *> lights);

	uint32_t get_point_light_count() const;

	backend::Buffer &get_light_camera_spheres_buffer() const;
	backend::Buffer &get_light_camera_matrices_buffer() const;

	const PointLightUniform &get_point_light_uniform() const;

  private:
	PointShadowsResources() = default;

	bool               is_initialized_{false};
	static inline bool destroyed_{false};

	PointLightUniform point_light_uniform_;

	std::unique_ptr<backend::Buffer> light_camera_spheres_buffer_;
	std::unique_ptr<backend::Buffer> light_camera_matrices_buffer_;

	uint32_t point_light_count_{0};
};

class PointShadowsCullingPass : public RenderPass
{
  public:
	PointShadowsCullingPass(GpuScene &gpu_scene, std::vector<sg::Light *> lights);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:
	GpuScene &gpu_scene_;
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
	PointShadowsPass(GpuScene &gpu_scene, std::vector<sg::Light *> lights);

	~PointShadowsPass() override;

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:
	GpuScene &gpu_scene_;
};
}        // namespace xihe::rendering
