#include "shadow_subpass.h"

#include "rendering/render_context.h"
#include "rendering/render_frame.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"
#include "shared_uniform.h"

namespace xihe::rendering
{
std::unique_ptr<backend::Sampler> get_shadowmap_sampler(backend::Device &device)
{
	vk::SamplerCreateInfo shadowmap_sampler_create_info{};
	shadowmap_sampler_create_info.minFilter     = vk::Filter::eLinear;
	shadowmap_sampler_create_info.magFilter     = vk::Filter::eLinear;
	shadowmap_sampler_create_info.addressModeU  = vk::SamplerAddressMode::eClampToBorder;
	shadowmap_sampler_create_info.addressModeV  = vk::SamplerAddressMode::eClampToBorder;
	shadowmap_sampler_create_info.addressModeW  = vk::SamplerAddressMode::eClampToBorder;
	shadowmap_sampler_create_info.borderColor   = vk::BorderColor::eFloatOpaqueWhite;
	shadowmap_sampler_create_info.compareEnable = VK_TRUE;
	shadowmap_sampler_create_info.compareOp     = vk::CompareOp::eGreaterOrEqual;
	return std::make_unique<backend::Sampler>(device, shadowmap_sampler_create_info);
}

ShadowSubpass::ShadowSubpass(RenderContext          &render_context,
                             backend::ShaderSource &&vertex_source,
                             backend::ShaderSource &&fragment_source,
                             sg::Scene              &scene,
                             sg::CascadeScript      &cascade_script,
                             uint32_t                cascade_index) :
    Subpass{render_context, std::move(vertex_source), std::move(fragment_source)},
    meshes_{scene.get_components<sg::Mesh>()},
    cascade_script_{cascade_script},
    cascade_index_{cascade_index}
{
}

void ShadowSubpass::prepare()
{
	auto &device = render_context_.get_device();
	device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
}

void ShadowSubpass::draw(backend::CommandBuffer &command_buffer)
{
	auto                                &device             = command_buffer.get_device();
	auto                                &vert_shader_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module};
	auto                                &pipeline_layout = prepare_pipeline_layout(command_buffer, shader_modules);

	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto vertex_input_resources = pipeline_layout.get_resources(backend::ShaderResourceType::kInput, vk::ShaderStageFlagBits::eVertex);

	for (auto &mesh : meshes_)
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &sub_mesh : mesh->get_submeshes())
			{
				update_uniforms(command_buffer, *node, thread_index_);
				draw_submesh(command_buffer, *sub_mesh, vertex_input_resources);
			}
		}
	}
}

void ShadowSubpass::update_uniforms(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index)
{
	sg::OrthographicCamera &cascade_camera = cascade_script_.get_cascade_camera(cascade_index_);

	SceneUniform global_uniform{};

	global_uniform.camera_view_proj = cascade_camera.get_pre_rotation() * xihe::vulkan_style_projection(cascade_camera.get_projection()) * cascade_camera.get_view();

	// shadow_uniform_.shadowmap_projection_matrix[cascade_index_] = vulkan_style_projection(cascade_camera_->get_projection()) * cascade_camera_->get_view();

	auto &render_frame = render_context_.get_active_frame();

	auto &transform = node.get_transform();

	auto allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SceneUniform), thread_index);

	global_uniform.model           = transform.get_world_matrix();
	global_uniform.camera_position = glm::vec3((glm::inverse(cascade_camera.get_view())[3]));

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
}

void ShadowSubpass::draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, const std::vector<backend::ShaderResource> &vertex_input_resources)
{
	prepare_pipeline_state(command_buffer, vk::FrontFace::eClockwise);

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

void ShadowSubpass::prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face) const
{
	RasterizationState rasterization_state;
	rasterization_state.front_face        = front_face;
	rasterization_state.depth_bias_enable = VK_TRUE;
	// It is necessary to specify the desired features before creating the device.
	// In this framework, you can enable the depthClamp feature by setting it to VK_TRUE
	// using: gpu.get_mutable_requested_features().depthClamp = VK_TRUE;
	rasterization_state.depth_clamp_enable = VK_TRUE;

	command_buffer.set_rasterization_state(rasterization_state);
	command_buffer.set_depth_bias(-1.4f, 0.0f, -1.7f);

	MultisampleState multisample_state{};
	multisample_state.rasterization_samples = sample_count_;
	command_buffer.set_multisample_state(multisample_state);
}

backend::PipelineLayout &ShadowSubpass::prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules)
{
	// Only vertex shader is needed in the shadow subpass
	assert(!shader_modules.empty());
	auto vertex_shader_module = shader_modules[0];

	vertex_shader_module->set_resource_mode("GlobalUniform", backend::ShaderResourceMode::kDynamic);

	return command_buffer.get_device().get_resource_cache().request_pipeline_layout({vertex_shader_module});
}

}        // namespace xihe::rendering
