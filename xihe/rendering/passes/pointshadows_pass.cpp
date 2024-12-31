#include "pointshadows_pass.h"

namespace xihe::rendering
{
PointShadowsResources &PointShadowsResources::get()
{
	static PointShadowsResources instance;
	return instance;
}

void PointShadowsResources::destroy()
{
	if (destroyed_)
	{
		return;
	}

	auto &instance = get();
	instance.light_camera_spheres_buffer_.reset();
	instance.light_camera_matrices_buffer_.reset();

	destroyed_ = true;
}

void PointShadowsResources::initialize(backend::Device &device, std::vector<sg::Light *> lights)
{
	if (is_initialized_ || destroyed_)
	{
		return;
	}

	std::vector<glm::vec4> light_camera_spheres;
	std::vector<glm::mat4> light_camera_matrices;

	point_light_count_ = 0;

	for (auto &light : lights)
	{
		if (light->get_light_type() == sg::LightType::kPoint)
		{
			PointLight point_light;
			point_light.position  = light->get_node()->get_transform().get_translation();
			point_light.radius    = light->get_properties().range;
			point_light.color     = light->get_properties().color;
			point_light.intensity = light->get_properties().intensity;

			light_camera_spheres.emplace_back(point_light.position, point_light.radius);

			for (uint32_t i = 0; i < 6; ++i)
			{
				glm::vec3 look_at;
				glm::vec3 up;
				switch (i)
				{
					case 0:
						look_at = glm::vec3(1.0f, 0.0f, 0.0f);
						up      = glm::vec3(0.0f, -1.0f, 0.0f);
						break;
					case 1:
						look_at = glm::vec3(-1.0f, 0.0f, 0.0f);
						up      = glm::vec3(0.0f, -1.0f, 0.0f);
						break;
					case 2:
						look_at = glm::vec3(0.0f, 1.0f, 0.0f);
						up      = glm::vec3(0.0f, 0.0f, 1.0f);
						break;
					case 3:
						look_at = glm::vec3(0.0f, -1.0f, 0.0f);
						up      = glm::vec3(0.0f, 0.0f, -1.0f);
						break;
					case 4:
						look_at = glm::vec3(0.0f, 0.0f, 1.0f);
						up      = glm::vec3(0.0f, -1.0f, 0.0f);
						break;
					case 5:
						look_at = glm::vec3(0.0f, 0.0f, -1.0f);
						up      = glm::vec3(0.0f, -1.0f, 0.0f);
						break;
				}

				glm::mat4 view = glm::lookAt(point_light.position, point_light.position + look_at, up);
				glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, point_light.radius, 0.1f);
				proj           = vulkan_style_projection(proj);

				light_camera_matrices.push_back(proj * view);
			}

			point_light_uniform_.point_lights[point_light_count_] = point_light;
			++point_light_count_;
		}
	}

	{
		backend::BufferBuilder light_camera_spheres_builder{light_camera_spheres.size() * sizeof(glm::vec4)};
		light_camera_spheres_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		light_camera_spheres_buffer_ = light_camera_spheres_builder.build_unique(device);
		light_camera_spheres_buffer_->set_debug_name("camera spheres");
		light_camera_spheres_buffer_->update(light_camera_spheres);
	}

	{
		backend::BufferBuilder light_camera_matrices_builder{light_camera_matrices.size() * sizeof(glm::mat4)};
		light_camera_matrices_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		light_camera_matrices_buffer_ = light_camera_matrices_builder.build_unique(device);
		light_camera_matrices_buffer_->set_debug_name("camera matrices");
		light_camera_matrices_buffer_->update(light_camera_matrices);
	}

	is_initialized_ = true;
}

uint32_t PointShadowsResources::get_point_light_count() const
{
	return point_light_count_;
}

backend::Buffer &PointShadowsResources::get_light_camera_spheres_buffer() const
{
	return *light_camera_spheres_buffer_;
}

backend::Buffer &PointShadowsResources::get_light_camera_matrices_buffer() const
{
	return *light_camera_matrices_buffer_;
}

const PointLightUniform &PointShadowsResources::get_point_light_uniform() const
{
	return point_light_uniform_;
}

PointShadowsCullingPass::PointShadowsCullingPass(GpuScene &gpu_scene, std::vector<sg::Light *> lights) :
    gpu_scene_{gpu_scene}
{
	PointShadowsResources::get().initialize(gpu_scene_.get_device(), lights);
}

void PointShadowsCullingPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&comp_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(PointLightUniform), thread_index_);
	allocation.update(PointShadowsResources::get().get_point_light_uniform());
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);

	command_buffer.bind_buffer(gpu_scene_.get_global_meshlet_buffer(), 0, gpu_scene_.get_global_meshlet_buffer().get_size(), 0, 2, 0);
	command_buffer.bind_buffer(gpu_scene_.get_mesh_draws_buffer(), 0, gpu_scene_.get_mesh_draws_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(gpu_scene_.get_instance_buffer(), 0, gpu_scene_.get_instance_buffer().get_size(), 0, 4, 0);
	command_buffer.bind_buffer(gpu_scene_.get_mesh_bounds_buffer(), 0, gpu_scene_.get_mesh_bounds_buffer().get_size(), 0, 5, 0);

	command_buffer.bind_buffer(input_bindables[0].buffer(), 0, input_bindables[0].buffer().get_size(), 0, 20, 0);
	command_buffer.bind_buffer(input_bindables[1].buffer(), 0, input_bindables[1].buffer().get_size(), 0, 21, 0);

	command_buffer.bind_buffer(PointShadowsResources::get().get_light_camera_spheres_buffer(), 0, PointShadowsResources::get().get_light_camera_spheres_buffer().get_size(), 0, 23, 0);

	command_buffer.set_specialization_constant(0, to_u32(PointShadowsResources::get().get_point_light_count()));
	command_buffer.set_specialization_constant(1, to_u32(gpu_scene_.get_instance_count()));

	command_buffer.clear_buffer(input_bindables[1].buffer());

	command_buffer.dispatch((PointShadowsResources::get().get_point_light_count() + 7) / 8, (gpu_scene_.get_instance_count() + 7) / 8, 1);
}

void PointShadowsCommandsGenerationPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&comp_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	command_buffer.bind_buffer(input_bindables[0].buffer(), 0, input_bindables[0].buffer().get_size(), 0, 21, 0);
	command_buffer.bind_buffer(input_bindables[1].buffer(), 0, input_bindables[1].buffer().get_size(), 0, 22, 0);

	command_buffer.set_specialization_constant(0, PointShadowsResources::get().get_point_light_count());

	command_buffer.dispatch((PointShadowsResources::get().get_point_light_count() + 31) / 32, 1, 1);
}

PointShadowsPass::PointShadowsPass(GpuScene &gpu_scene, std::vector<sg::Light *> lights) :
    gpu_scene_{gpu_scene}
{
	PointShadowsResources::get().initialize(gpu_scene_.get_device(), lights);
}

PointShadowsPass::~PointShadowsPass()
{
	PointShadowsResources::destroy();
}

void PointShadowsPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	command_buffer.set_has_mesh_shader(true);

	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &task_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eTaskEXT, get_task_shader());
	auto &mesh_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader());

	std::vector<backend::ShaderModule *> shader_modules{&task_shader_module, &mesh_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	DepthStencilState depth_stencil_state{};
	depth_stencil_state.depth_test_enable  = true;
	depth_stencil_state.depth_write_enable = true;

	command_buffer.set_depth_stencil_state(depth_stencil_state);

	command_buffer.bind_buffer(gpu_scene_.get_mesh_draws_buffer(), 0, gpu_scene_.get_mesh_draws_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(gpu_scene_.get_instance_buffer(), 0, gpu_scene_.get_instance_buffer().get_size(), 0, 4, 0);

	command_buffer.bind_buffer(gpu_scene_.get_global_meshlet_buffer(), 0, gpu_scene_.get_global_meshlet_buffer().get_size(), 0, 7, 0);
	command_buffer.bind_buffer(gpu_scene_.get_global_vertex_buffer(), 0, gpu_scene_.get_global_vertex_buffer().get_size(), 0, 8, 0);
	command_buffer.bind_buffer(gpu_scene_.get_global_meshlet_vertices_buffer(), 0, gpu_scene_.get_global_meshlet_vertices_buffer().get_size(), 0, 9, 0);
	command_buffer.bind_buffer(gpu_scene_.get_global_packed_meshlet_indices_buffer(), 0, gpu_scene_.get_global_packed_meshlet_indices_buffer().get_size(), 0, 10, 0);

	command_buffer.bind_buffer(input_bindables[0].buffer(), 0, input_bindables[0].buffer().get_size(), 0, 20, 0);
	command_buffer.bind_buffer(input_bindables[1].buffer(), 0, input_bindables[1].buffer().get_size(), 0, 22, 0);

	command_buffer.bind_buffer(PointShadowsResources::get().get_light_camera_spheres_buffer(), 0, PointShadowsResources::get().get_light_camera_spheres_buffer().get_size(), 0, 23, 0);
	command_buffer.bind_buffer(PointShadowsResources::get().get_light_camera_matrices_buffer(), 0, PointShadowsResources::get().get_light_camera_matrices_buffer().get_size(), 0, 24, 0);

	command_buffer.draw_mesh_tasks_indirect(input_bindables[1].buffer(), 0, PointShadowsResources::get().get_point_light_count() * 6, sizeof(glm::uvec4));

	command_buffer.set_has_mesh_shader(false);
}
}        // namespace xihe::rendering
