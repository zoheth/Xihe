#pragma once

#include "common/glm_common.h"
#include "rendering/subpass.h"

enum
{
	MAX_DEFERRED_LIGHT_COUNT = 32
};

namespace xihe
{
namespace sg
{
class Camera;
class Light;
class Scene;
}        // namespace sg

namespace rendering
{
struct alignas(16) LightUniform
{
	glm::mat4 inv_view_proj;
	glm::vec2 inv_resolution;
};

struct alignas(16) DeferredLights
{
	Light directional_lights[MAX_DEFERRED_LIGHT_COUNT];
	Light point_lights[MAX_DEFERRED_LIGHT_COUNT];
	Light spot_lights[MAX_DEFERRED_LIGHT_COUNT];
};

class LightingSubpass : public Subpass
{
  public:
	LightingSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader, sg::Camera &camera, sg::Scene &scene);

	void prepare() override;

	void draw(backend::CommandBuffer &command_buffer) override;

  private:
	sg::Camera &camera_;

	sg::Scene &scene_;

	backend::ShaderVariant shader_variant_;
};

}        // namespace rendering

}        // namespace xihe