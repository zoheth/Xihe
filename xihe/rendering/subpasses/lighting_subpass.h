#pragma once

#include "common/glm_common.h"
#include "rendering/subpass.h"
#include "scene_graph/scripts/cascade_script.h"

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
class CascadeScript;
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

struct alignas(16) ShadowUniform
{
	float                                cascade_split_depth[4];             // Split depths in view space
	std::array<glm::mat4, kCascadeCount> shadowmap_projection_matrix;        // Projection matrix used to render shadowmap
	uint32_t                             shadowmap_first_index;              // Index of the first shadowmap in the bindless texture array
};

class LightingSubpass : public Subpass
{
  public:
	LightingSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader, sg::Camera &camera, sg::Scene &scene, sg::CascadeScript *cascade_script_ptr);

	void prepare() override;

	void draw(backend::CommandBuffer &command_buffer) override;

  private:
	sg::Camera &camera_;

	sg::Scene &scene_;

	sg::CascadeScript *cascade_script_;

	backend::ShaderVariant shader_variant_;

	std::unique_ptr<backend::Sampler> shadowmap_sampler_{};
};

}        // namespace rendering

}        // namespace xihe