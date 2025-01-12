#include "skybox_pass.h"

#include "geometry_pass.h"
#include "scene_graph/components/image.h"

namespace xihe::rendering
{
SkyboxPass::SkyboxPass(sg::Mesh &box_mesh, Texture &cubemap, sg::Camera &camera) :
    box_mesh_(box_mesh), cubemap_(&cubemap), camera_(camera)
{}

SkyboxPass::SkyboxPass(sg::Mesh &box_mesh, sg::Texture &cubemap, sg::Camera &camera) :
    box_mesh_(box_mesh), cubemap_sg_(&cubemap), camera_(camera)
{}

void SkyboxPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader());

	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);

	command_buffer.bind_pipeline_layout(pipeline_layout);

	RasterizationState rasterization_state;
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	auto mvp        = vulkan_style_projection(camera_.get_projection()) * glm::mat4(glm::mat3(camera_.get_view()));
	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(glm::mat4), thread_index_);
	allocation.update(mvp);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 0, 0);

	if (cubemap_)
	{
		command_buffer.bind_image(*cubemap_->image_view, *cubemap_->sampler, 0, 2, 0);
	}
	else
	{
		command_buffer.bind_image(cubemap_sg_->get_image()->get_vk_image_view(), cubemap_sg_->get_sampler()->vk_sampler_, 0, 2, 0);
	}

	sg::SubMesh &sub_mesh = *box_mesh_.get_submeshes()[0];
	bind_submesh_vertex_buffers(command_buffer, pipeline_layout, sub_mesh);

	if (sub_mesh.index_count != 0)
	{
		command_buffer.bind_index_buffer(*sub_mesh.index_buffer, sub_mesh.index_offset, sub_mesh.index_type);

		command_buffer.draw_indexed(sub_mesh.index_count, 1, 0, 0, 0);
	}
	else
	{
		command_buffer.draw(sub_mesh.vertex_count, 1, 0, 0);
	}
}
}        // namespace xihe::rendering
