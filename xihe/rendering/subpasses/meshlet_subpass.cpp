#include "meshlet_subpass.h"

#include "forward_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/mesh.h"
#include "rendering/render_context.h"
#include "scene_graph/scene.h"
#include "shared_uniform.h"

constexpr uint32_t kMaxForwardLightCount = 4;

namespace xihe::rendering
{
MeshletSubpass::MeshletSubpass(rendering::RenderContext &render_context, std::optional<backend::ShaderSource> task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera) :
    Subpass{render_context, std::move(task_shader), std::move(mesh_shader), std::move(fragment_shader)},
	camera_{camera},
	scene_{scene}
{
	mesh_         = scene.get_components<sg::Mesh>()[0];
	mshader_mesh_ = mesh_->get_mshader_meshes()[0];
	assert(mshader_mesh_);
}

void MeshletSubpass::prepare()
{
	auto &device  = get_render_context().get_device();
	auto &variant = mshader_mesh_->get_mut_shader_variant();
	variant.add_definitions({"MAX_LIGHT_COUNT " + std::to_string(kMaxForwardLightCount)});

	variant.add_definitions(kLightTypeDefinitions);

	auto &mesh_shader_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader(), variant);
	auto &frag_shader_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), variant);
}
void MeshletSubpass::draw(backend::CommandBuffer &command_buffer)
{
	//RasterizationState rasterization_state;
	//rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	//command_buffer.set_rasterization_state(rasterization_state);

	allocate_lights<ForwardLights>(scene_.get_components<sg::Light>(), kMaxForwardLightCount);
	command_buffer.bind_lighting(get_lighting_state(), 1, 0);

	command_buffer.set_has_mesh_shader();

	update_uniform(command_buffer, 0);

	auto &mesh_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader(), mshader_mesh_->get_shader_variant());
	auto &frag_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), mshader_mesh_->get_shader_variant());

	std::vector<backend::ShaderModule *> shader_modules{&mesh_shader_module, &frag_shader_module};

	auto &pipeline_layout = command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	command_buffer.bind_buffer(mshader_mesh_->get_meshlet_buffer(), 0, mshader_mesh_->get_meshlet_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(mshader_mesh_->get_vertex_buffer(), 0, mshader_mesh_->get_vertex_buffer().get_size(), 0, 4, 0);
	command_buffer.bind_buffer(mshader_mesh_->get_meshlet_vertices_buffer(), 0, mshader_mesh_->get_meshlet_vertices_buffer().get_size(), 0, 5, 0);
	command_buffer.bind_buffer(mshader_mesh_->get_packed_meshlet_indices_buffer(), 0, mshader_mesh_->get_packed_meshlet_indices_buffer().get_size(), 0, 6, 0);

	uint32_t num_workgroups_x = mshader_mesh_->get_meshlet_count();        // meshlets count
	uint32_t num_workgroups_y = 1;
	uint32_t num_workgroups_z = 1;
	command_buffer.draw_mesh_tasks(num_workgroups_x, num_workgroups_y, num_workgroups_z);
}
void MeshletSubpass::update_uniform(backend::CommandBuffer &command_buffer, size_t thread_index) const
{
	SceneUniform global_uniform{};
	global_uniform.camera_view_proj = camera_.get_pre_rotation() * xihe::vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

	global_uniform.camera_position = glm::vec3((glm::inverse(camera_.get_view())[3]));

	global_uniform.model = mesh_->get_nodes()[0]->get_transform().get_world_matrix();

	auto &render_frame = render_context_.get_active_frame();
	auto  allocation   = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SceneUniform), thread_index);

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 2, 0);
}
}        // namespace xihe::rendering