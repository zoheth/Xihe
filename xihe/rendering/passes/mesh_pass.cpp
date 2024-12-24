#include "mesh_pass.h"

namespace xihe::rendering
{

namespace
{
glm::vec4 normalize_plane(const glm::vec4 &plane)
{
	float length = glm::length(glm::vec3(plane.x, plane.y, plane.z));
	return plane / length;
}
}        // namespace

MeshPass::MeshPass(GpuScene &gpu_scene, sg::Camera &camera) :

    gpu_scene_{gpu_scene},
    camera_{camera}
{}

void MeshPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	command_buffer.set_has_mesh_shader(true);

	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &task_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eTaskEXT, get_task_shader(), shader_variant_);
	auto &mesh_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader(), shader_variant_);
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), shader_variant_);

	std::vector<backend::ShaderModule *> shader_modules{&task_shader_module, &mesh_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules, &resource_cache.request_bindless_descriptor_set());
	command_buffer.bind_pipeline_layout(pipeline_layout);

	DepthStencilState depth_stencil_state{};
	depth_stencil_state.depth_test_enable  = true;
	depth_stencil_state.depth_write_enable = true;

	command_buffer.set_depth_stencil_state(depth_stencil_state);

	MeshSceneUniform global_uniform{};
	global_uniform.camera_view_proj = camera_.get_pre_rotation() * vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

	global_uniform.camera_position = glm::vec3((glm::inverse(camera_.get_view())[3]));

	if (freeze_frustum_)
	{
		global_uniform.view              = frozen_view_;
		global_uniform.frustum_planes[0] = frozen_frustum_planes_[0];
		global_uniform.frustum_planes[1] = frozen_frustum_planes_[1];
		global_uniform.frustum_planes[2] = frozen_frustum_planes_[2];
		global_uniform.frustum_planes[3] = frozen_frustum_planes_[3];
		global_uniform.frustum_planes[4] = frozen_frustum_planes_[4];
		global_uniform.frustum_planes[5] = frozen_frustum_planes_[5];
	}
	else
	{
		global_uniform.view              = camera_.get_view();
		glm::mat4 m                      = glm::transpose(camera_.get_pre_rotation() * camera_.get_projection());
		global_uniform.frustum_planes[0] = normalize_plane(m[3] + m[0]);
		global_uniform.frustum_planes[1] = normalize_plane(m[3] - m[0]);
		global_uniform.frustum_planes[2] = normalize_plane(m[3] + m[1]);
		global_uniform.frustum_planes[3] = normalize_plane(m[3] - m[1]);
		global_uniform.frustum_planes[4] = normalize_plane(m[3] + m[2]);
		global_uniform.frustum_planes[5] = normalize_plane(m[3] - m[2]);
	}

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(MeshSceneUniform), thread_index_);

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 2, 0);

	command_buffer.bind_buffer(gpu_scene_.get_mesh_draws_buffer(), 0, gpu_scene_.get_mesh_draws_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(gpu_scene_.get_instance_buffer(), 0, gpu_scene_.get_instance_buffer().get_size(), 0, 4, 0);
	command_buffer.bind_buffer(gpu_scene_.get_draw_command_buffer(), 0, gpu_scene_.get_draw_command_buffer().get_size(), 0, 5, 0);

	command_buffer.bind_buffer(gpu_scene_.get_global_meshlet_buffer(), 0, gpu_scene_.get_global_meshlet_buffer().get_size(), 0, 7, 0);
	command_buffer.bind_buffer(gpu_scene_.get_global_vertex_buffer(), 0, gpu_scene_.get_global_vertex_buffer().get_size(), 0, 8, 0);
	command_buffer.bind_buffer(gpu_scene_.get_global_meshlet_vertices_buffer(), 0, gpu_scene_.get_global_meshlet_vertices_buffer().get_size(), 0, 9, 0);
	command_buffer.bind_buffer(gpu_scene_.get_global_packed_meshlet_indices_buffer(), 0, gpu_scene_.get_global_packed_meshlet_indices_buffer().get_size(), 0, 10, 0);

	command_buffer.draw_mesh_tasks_indirect_count(gpu_scene_.get_draw_command_buffer(), 0, gpu_scene_.get_draw_counts_buffer(), 0, gpu_scene_.get_instance_count(), sizeof(MeshDrawCommand));

	command_buffer.set_has_mesh_shader(false);
}

void MeshPass::show_meshlet_view(bool show)
{
	if (show == show_debug_view_)
	{
		return;
	}
	show_debug_view_ = show;

	if (show)
	{
		shader_variant_.add_define("SHOW_MESHLET_VIEW");
	}
	else
	{
		shader_variant_.remove_define("SHOW_MESHLET_VIEW");
	}
}

void MeshPass::freeze_frustum(bool freeze, sg::Camera *camera)
{
	assert(camera);
	if (freeze == freeze_frustum_)
	{
		return;
	}
	freeze_frustum_ = freeze;
	if (freeze)
	{
		frozen_view_              = camera->get_view();
		glm::mat4 m               = glm::transpose(camera->get_pre_rotation() * camera->get_projection());
		frozen_frustum_planes_[0] = normalize_plane(m[3] + m[0]);
		frozen_frustum_planes_[1] = normalize_plane(m[3] - m[0]);
		frozen_frustum_planes_[2] = normalize_plane(m[3] + m[1]);
		frozen_frustum_planes_[3] = normalize_plane(m[3] - m[1]);
		frozen_frustum_planes_[4] = normalize_plane(m[3] + m[2]);
		frozen_frustum_planes_[5] = normalize_plane(m[3] - m[2]);
	}
}
}        // namespace xihe::rendering
