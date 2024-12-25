#include "pointshadows_pass.h"

namespace xihe::rendering
{
PointShadowsCullingPass::PointShadowsCullingPass(GpuScene &gpu_scene, std::vector<sg::Light *> lights) :
    gpu_scene_{gpu_scene}
{

	for (auto &light : lights)
	{
		if (light->get_light_type() == sg::LightType::kPoint)
		{
			PointLight point_light;
			point_light.position  = light->get_node()->get_transform().get_translation();
			point_light.radius    = light->get_properties().range;
			point_light.color     = light->get_properties().color;
			point_light.intensity = light->get_properties().intensity;
			point_light_uniform_.point_lights[point_light_count_] = point_light;
			++point_light_count_;
		}
	}
}

void PointShadowsCullingPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&comp_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer,  sizeof(PointLightUniform), thread_index_);
	allocation.update(point_light_uniform_);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);

	command_buffer.bind_buffer(gpu_scene_.get_global_meshlet_buffer(), 0, gpu_scene_.get_global_meshlet_buffer().get_size(), 0, 2, 0);
	command_buffer.bind_buffer(gpu_scene_.get_mesh_draws_buffer(), 0, gpu_scene_.get_mesh_draws_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(gpu_scene_.get_instance_buffer(), 0, gpu_scene_.get_instance_buffer().get_size(), 0, 4, 0);

	command_buffer.bind_buffer(input_bindables[0].buffer(), 0, input_bindables[0].buffer().get_size(), 0, 20, 0);
	command_buffer.bind_buffer(input_bindables[1].buffer(), 0, input_bindables[1].buffer().get_size(), 0, 21, 0);

	command_buffer.set_specialization_constant(0, to_u32(point_light_count_));
	command_buffer.set_specialization_constant(1, to_u32(gpu_scene_.get_instance_count()));

	command_buffer.dispatch(((point_light_count_ * gpu_scene_.get_instance_count()) + 31) / 32, 1, 1);
}
}        // namespace xihe::rendering
