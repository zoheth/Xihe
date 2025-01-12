#include "preprocess.h"

#include "geometry_pass.h"
#include "scene_graph/components/image.h"

namespace xihe::rendering
{
namespace
{
std::vector<glm::mat4> matrices = {
    glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
};
}

PrefilterPass::PrefilterPass(sg::Mesh &sky_box, sg::Texture &cubemap, uint32_t mip, uint32_t face) :
    cubemap_(cubemap), sky_box_(sky_box), mip_(mip), face_(face)
{}

void PrefilterPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader());

	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);

	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto  mvp       = glm::perspective(static_cast<float>((glm::pi<float>() / 2.0)), 1.0f, 0.1f, 512.0f) * matrices[face_];
	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(glm::mat4), thread_index_);
	allocation.update(mvp);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 0, 0);

	const float roughness = static_cast<float>(mip_) / static_cast<float>(num_mips - 1);
	command_buffer.set_specialization_constant(0, roughness);
	command_buffer.set_specialization_constant(1, 32U);        // num_samples


	command_buffer.bind_image(cubemap_.get_image()->get_vk_image_view(), cubemap_.get_sampler()->vk_sampler_, 0, 1, 0);

	sg::SubMesh &sub_mesh = *sky_box_.get_submeshes()[0];
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
