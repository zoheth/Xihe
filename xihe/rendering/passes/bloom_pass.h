#pragma once

#include "render_pass.h"

namespace xihe::rendering
{
struct ExtractPush
{
	float threshold = 1.0f;
	float knee      = 0.5f;
	float exposure  = 1.2f;
};

struct BlurPush
{
	float inner_weight = 0.75f;
	float outer_weight = 0.25f;
	float inner_offset = 1.0f;
	float outer_offset = 2.0f;
};

struct CompositePush
{
	float intensity  = 0.8f;
	float tint_r     = 1.0f;
	float tint_g     = 1.0f;
	float tint_b     = 1.0f;
	float saturation = 1.0f;
};

class BloomExtractPass : public RenderPass
{
  public:
	BloomExtractPass() = default;
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;
};

class BloomBlurPass : public RenderPass
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