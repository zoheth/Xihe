#pragma once

#include "render_pass.h"

namespace xihe::rendering
{
class ScreenPass : public RenderPass
{
  public:
	ScreenPass();
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

};
}
