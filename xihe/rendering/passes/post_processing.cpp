#include "post_processing.h"

namespace xihe::rendering
{
void ScreenPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	RenderPass::execute(command_buffer, active_frame, input_bindables);
}
}
