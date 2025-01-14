#pragma once

#include "render_pass.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/texture.h"

namespace xihe::rendering
{

enum PreprocessType
{
	kIrradiance = 0,
	kPrefilter = 1,
};

class PrefilterPass : public RenderPass
{
  public:
	PrefilterPass(sg::SubMesh &sky_box, sg::Texture &cubemap, uint32_t mip, uint32_t face, PreprocessType target);

	~PrefilterPass() override = default;

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

	inline static uint32_t num_mips = 0;

  private:
	PreprocessType target_;

	sg::Texture &cubemap_;
	sg::SubMesh    &sky_box_;

	uint32_t mip_{};
	uint32_t face_{};
};


class BrdfLutPass : public RenderPass
{
  public:
	BrdfLutPass() = default;
	auto execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) -> void override;
};
}
