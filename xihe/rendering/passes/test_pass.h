#pragma once

#include "render_pass.h"

namespace xihe::rendering
{
class TestPass : public RenderPass
{
  public:
	TestPass() = default;
	~TestPass() override = default;
	auto execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) -> void override;
};
}
