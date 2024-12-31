#include "clear_pass.h"

namespace xihe::rendering
{
void ClearPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&comp_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	command_buffer.bind_buffer(input_bindables[0].buffer(), 0, input_bindables[0].buffer().get_size(), 0, 0, 0);

	command_buffer.dispatch((256 * 25) / 64, 1, 1);
}
}        // namespace xihe::rendering
