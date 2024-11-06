#include "gpu_scene_subpass.h"

#include "shared_uniform.h"

namespace xihe::rendering
{
void GpuSceneSubpass::draw(backend::CommandBuffer &command_buffer)
{
	command_buffer.set_has_mesh_shader(true);

	auto &task_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eTaskEXT, get_task_shader());
	auto &mesh_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader());
	auto &frag_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader());

	std::vector<backend::ShaderModule *> shader_modules{&task_shader_module, &mesh_shader_module, &frag_shader_module};

	auto &pipeline_layout = command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);


	command_buffer.draw_mesh_tasks(1, 1, 1);

	command_buffer.set_has_mesh_shader(false);
}
}
