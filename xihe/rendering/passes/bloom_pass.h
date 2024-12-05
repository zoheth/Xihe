#pragma once

#include "render_pass.h"

namespace xihe::rendering
{
struct Push
{
	uint32_t width, height;
	float    inv_width, inv_height;
	float    inv_input_width, inv_input_height;
};

class BloomScreenPass : public RenderPass
{
  public:
	BloomScreenPass() = default;
	auto execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) -> void override;
};

}        // namespace xihe::rendering