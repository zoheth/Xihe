#include "mesh_shading_test_subpass.h"

#include "rendering/render_context.h"

namespace xihe::rendering
{
MeshShadingTestSubpass::MeshShadingTestSubpass(rendering::RenderContext &render_context, backend::ShaderSource &&task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader) :
    Subpass{render_context, std::move(task_shader), std::move(mesh_shader), std::move(fragment_shader)}
{}

void MeshShadingTestSubpass::prepare()
{}

void MeshShadingTestSubpass::draw(backend::CommandBuffer &command_buffer)
{
	command_buffer.set_has_mesh_shader(true);

	RasterizationState rasterization_state{};
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	update_uniform(command_buffer, 0);

	auto &task_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eTaskEXT, get_task_shader());
	auto &mesh_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader());
	auto &frag_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader());

	std::vector<backend::ShaderModule *> shader_modules{&task_shader_module, &mesh_shader_module, &frag_shader_module};

	auto &pipeline_layout = command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	uint32_t N             = density_level_ == 0 ? 4 : (density_level_ == 1 ? 6 : (density_level_ == 2 ? 8 : 2));
	uint32_t group_count_x = N;
	uint32_t group_count_y = N;

	command_buffer.draw_mesh_tasks(group_count_x, group_count_y, 1);

	command_buffer.set_has_mesh_shader(false);
}

void MeshShadingTestSubpass::update_uniform(backend::CommandBuffer &command_buffer, size_t thread_index) const
{
	auto &render_frame = render_context_.get_active_frame();
	auto  allocation   = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(CullUniform), thread_index);

	allocation.update(cull_uniform_);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 0, 0);
}
}        // namespace xihe::rendering
