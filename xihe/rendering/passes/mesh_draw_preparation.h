#pragma once

#include "render_pass.h"

namespace xihe::rendering
{
class MeshDrawPreparationPass : public RenderPass
{
public:
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;
};
}
