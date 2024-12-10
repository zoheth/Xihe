#include "cascade_shadow_pass.h"

namespace xihe::rendering
{
CascadeShadowPass::CascadeShadowPass(std::vector<sg::Mesh *> meshes, sg::CascadeScript &cascade_script, uint32_t cascade_index) :
    meshes_{std::move(meshes)}, cascade_script_{cascade_script}, cascade_index_(cascade_index)
{}

void CascadeShadowPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
	vert_shader_module.set_resource_mode("GlobalUniform", backend::ShaderResourceMode::kDynamic);
	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	RasterizationState rasterization_state;
	rasterization_state.front_face        = vk::FrontFace::eClockwise;
	rasterization_state.depth_bias_enable = VK_TRUE;
	// It is necessary to specify the desired features before creating the device.
	// In this framework, you can enable the depthClamp feature by setting it to VK_TRUE
	// using: gpu.get_mutable_requested_features().depthClamp = VK_TRUE;
	rasterization_state.depth_clamp_enable = VK_TRUE;
	command_buffer.set_rasterization_state(rasterization_state);
	command_buffer.set_depth_bias(-1.4f, 0.0f, -1.7f);

	DepthStencilState depth_stencil_state{};
	depth_stencil_state.depth_test_enable  = true;
	depth_stencil_state.depth_write_enable = true;
	command_buffer.set_depth_stencil_state(depth_stencil_state);

	auto vertex_input_resources = pipeline_layout.get_resources(backend::ShaderResourceType::kInput, vk::ShaderStageFlagBits::eVertex);

	for (auto &mesh : meshes_)
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &sub_mesh : mesh->get_submeshes())
			{
				update_uniforms(command_buffer, active_frame, *node, thread_index_);
				draw_submesh(command_buffer, *sub_mesh, vertex_input_resources);
			}
		}
	}
}

void CascadeShadowPass::update_uniforms(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, sg::Node &node, size_t thread_index)
{
	sg::OrthographicCamera &cascade_camera = cascade_script_.get_cascade_camera(cascade_index_);

	SceneUniform global_uniform{};

	global_uniform.camera_view_proj = cascade_camera.get_pre_rotation() * vulkan_style_projection(cascade_camera.get_projection()) * cascade_camera.get_view();

	// shadow_uniform_.shadowmap_projection_matrix[cascade_index_] = vulkan_style_projection(cascade_camera_->get_projection()) * cascade_camera_->get_view();

	auto &transform = node.get_transform();

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SceneUniform), thread_index);

	global_uniform.model           = transform.get_world_matrix();
	global_uniform.camera_position = glm::vec3((glm::inverse(cascade_camera.get_view())[3]));

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
}

void CascadeShadowPass::draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, const std::vector<backend::ShaderResource> &vertex_input_resources)
{

	VertexInputState vertex_input_state{};

	for (auto &input_resource : vertex_input_resources)
	{
		VertexAttribute attribute;

		if (!sub_mesh.get_attribute(input_resource.name, attribute))
		{
			continue;
		}

		vk::VertexInputAttributeDescription vertex_attribute{
		    input_resource.location,
		    input_resource.location,
		    attribute.format,
		    attribute.offset};

		vertex_input_state.attributes.push_back(vertex_attribute);

		vk::VertexInputBindingDescription vertex_binding{
		    input_resource.location,
		    attribute.stride};

		vertex_input_state.bindings.push_back(vertex_binding);
	}

	command_buffer.set_vertex_input_state(vertex_input_state);

	for (auto &input_resource : vertex_input_resources)
	{
		const auto &buffer_iter = sub_mesh.vertex_buffers.find(input_resource.name);

		if (buffer_iter != sub_mesh.vertex_buffers.end())
		{
			std::vector<std::reference_wrapper<const backend::Buffer>> buffers;
			buffers.emplace_back(std::ref(buffer_iter->second));

			command_buffer.bind_vertex_buffers(input_resource.location, buffers, {0});
		}
	}

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
