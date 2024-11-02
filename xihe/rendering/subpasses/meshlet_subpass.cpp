#include "meshlet_subpass.h"

#include "scene_graph/components/camera.h"
#include "rendering/render_context.h"

namespace xihe::rendering
{
MeshletSubpass::MeshletSubpass(rendering::RenderContext &render_context, std::optional<backend::ShaderSource> task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera) :
    Subpass{render_context, std::move(task_shader), std::move(mesh_shader), std::move(fragment_shader)},
	camera_{camera},
	scene_{scene}
{
}

void MeshletSubpass::prepare()
{
}
void MeshletSubpass::draw(backend::CommandBuffer &command_buffer)
{
	command_buffer.set_has_mesh_shader();
}
void MeshletSubpass::update_uniform(backend::CommandBuffer &command_buffer, size_t thread_index) const
{
	UBOVS ubo{};
	ubo.proj  = camera_.get_projection();
	ubo.view  = camera_.get_view();
	ubo.model = glm::mat4(1.f);
	ubo.model = glm::rotate(ubo.model, glm::pi<float>(), glm::vec3(0, 0, 1));

	auto &render_frame = render_context_.get_active_frame();
	auto  allocation   = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UBOVS), thread_index);

	allocation.update(ubo);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 2, 0);
}
}        // namespace xihe::rendering