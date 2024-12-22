#include "mesh_draw_preparation.h"

namespace xihe::rendering
{
MeshDrawPreparationPass::MeshDrawPreparationPass(GpuScene &gpu_scene) :
    gpu_scene_(gpu_scene)
{}

void MeshDrawPreparationPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&comp_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	command_buffer.bind_buffer(gpu_scene_.get_mesh_draws_buffer(), 0, gpu_scene_.get_mesh_draws_buffer().get_size(), 0, 1, 0);
	command_buffer.bind_buffer(gpu_scene_.get_instance_buffer(), 0, gpu_scene_.get_instance_buffer().get_size(), 0, 2, 0);
	command_buffer.bind_buffer(gpu_scene_.get_draw_command_buffer(), 0, gpu_scene_.get_draw_command_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(gpu_scene_.get_draw_counts_buffer(), 0, gpu_scene_.get_draw_counts_buffer().get_size(), 0, 4, 0);



	command_buffer.dispatch((gpu_scene_.get_instance_count() + 255)/256, 1, 1);
}

}        // namespace xihe::rendering