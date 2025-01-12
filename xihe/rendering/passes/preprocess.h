#pragma once

#include "render_pass.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/texture.h"

namespace xihe::rendering
{
struct PushBlockIrradiance
{
	float     delta_phi   = (2.0f * glm::pi<float>()) / 180.0f;
	float delta_theta = (0.5f * glm::pi<float>()) / 64.0f;
};

struct PushBlockPrefilterEnv
{
	float     roughness;
	uint32_t  num_samples = 32u;
};

class PrefilterPass : public RenderPass
{
  public:
	PrefilterPass(sg::Mesh &sky_box, sg::Texture &cubemap, uint32_t mip, uint32_t face);

	~PrefilterPass() override = default;

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

	inline static uint32_t num_mips = 0;

  private:
	sg::Texture &cubemap_;
	sg::Mesh    &sky_box_;

	uint32_t mip_{};
	uint32_t face_{};
};
}
