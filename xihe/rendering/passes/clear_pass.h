#pragma once

#include "render_pass.h"

namespace xihe::rendering
{
class ClearPass : public RenderPass
{
  public:
	ClearPass()          = default;
	~ClearPass() override = default;
	auto execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) -> void override;
};
}        // namespace xihe::rendering
