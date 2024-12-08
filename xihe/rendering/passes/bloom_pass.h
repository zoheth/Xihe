#pragma once

#include "render_pass.h"

namespace xihe::rendering
{
struct alignas(16) CommonUniforms
{
	glm::uvec2 resolution;
	glm::vec2  inv_resolution;
	glm::vec2  inv_input_resolution;
};

struct ExtractPush
{
	float threshold      = 1.0f;
	float soft_threshold = 0.5f;
	float intensity      = 1.2f;
	float saturation     = 1.2f;
};

// struct BlurPush
//{
//	float inner_weight = 0.75f;
//	float outer_weight = 0.25f;
//	float inner_offset = 1.0f;
//	float outer_offset = 2.0f;
// };

struct BlurPush
{
	float inner_weight = 0.227027;
	float outer_weight = 0.1945946;
	int   is_vertical{0};
};

struct CompositePush
{
	float intensity  = 15.0f;
	float tint_r     = 1.0f;
	float tint_g     = 1.0f;
	float tint_b     = 1.0f;
	float saturation = 1.0f;
};

class BloomComputePass : public RenderPass
{
  public:
	BloomComputePass() = default;
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  protected:
	CommonUniforms uniforms_;
};

class BloomExtractPass : public BloomComputePass
{
  public:
	BloomExtractPass() = default;
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;
};

class BloomBlurPass : public BloomComputePass
{
  public:
	BloomBlurPass(bool horizontal);
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:
	bool horizontal_ = false;
};

class BloomCompositePass : public RenderPass
{
  public:
	BloomCompositePass() = default;
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;
};

}        // namespace xihe::rendering