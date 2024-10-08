#include "mesh_shading.h"

namespace xihe::rendering
{
void MeshShading::draw(backend::CommandBuffer &command_buffer)
{
	auto &task_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eTaskEXT, task_shader_);
	auto &mesh_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, mesh_shader_);
	auto &frag_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, fragment_shader_);

	std::vector<backend::ShaderModule *> shader_modules{&task_shader_module, &mesh_shader_module, &frag_shader_module};
	auto                                &pipeline_layout = prepare_pipeline_layout(command_buffer, shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	// todo bind descriptor set

	uint32_t N = density_level_ == 0 ? 4 : (density_level_ == 1 ? 6 : (density_level_ == 2 ? 8 : 2));
	// dispatch N * N task shader workgroups
	uint32_t group_count_x = N;
	uint32_t group_count_y = N;
	uint32_t group_count_z = 1;

	command_buffer.draw_mesh_tasks(group_count_x, group_count_x, 1);
}

backend::PipelineLayout &MeshShading::prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules)
{
	return command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules, render_context_.get_bindless_descriptor_set());
}
}        // namespace xihe::rendering
