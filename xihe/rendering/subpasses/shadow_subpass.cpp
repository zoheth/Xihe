#include "shadow_subpass.h"

#include "rendering/render_frame.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"

namespace xihe::rendering
{
ShadowSubpass::ShadowSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_source, backend::ShaderSource &&fragment_source, sg::Scene &scene, sg::PerspectiveCamera &camera, uint32_t cascade_index) :
    Subpass{render_context, std::move(vertex_source), std::move(fragment_source)},
    camera_{camera},
    meshes_{scene.get_components<sg::Mesh>()},
    scene_{scene},
	cascade_index_{cascade_index}
{
	auto       lights            = scene_.get_components<sg::Light>();
	sg::Light *directional_light = nullptr;
	for (auto &light : lights)
	{
		if (light->get_light_type() == sg::LightType::kDirectional)
		{
			directional_light = light;
			break;
		}
	}
	assert(cascade_index_<cascade_count_);

	cascade_camera_ = std::make_unique<sg::OrthographicCamera>("cascade_cameras_" + std::to_string(cascade_index_)+std::to_string(cascade_count_));
	cascade_camera_->set_node(*directional_light->get_node());

	if (cascade_splits_.size() != cascade_count_)
	{
		cascade_splits_.resize(cascade_count_);
		calculate_cascade_split_depth();
	}
}

void ShadowSubpass::calculate_cascade_split_depth(float lambda)
{

	float n = camera_.get_near_plane();
	float f = camera_.get_far_plane();
	for (uint32_t index = 0; index < cascade_count_; ++index)
	{
		float i = static_cast<float>(index);
		float N = static_cast<float>(cascade_count_);

		// Calculate the logarithmic and linear depth
		float c_log = n * std::pow((f / n), i / N);
		float c_lin = n + (i / N) * (f - n);

		// Interpolate between logarithmic and linear depth using lambda
		float c = lambda * c_log + (1 - lambda) * c_lin;

		// Convert view space depth to clip space depth for Vulkan's [0, 1] range.
		// The near and far planes are inverted in the projection matrix for precision reasons,
		// so we invert them here as well to match that convention.
		cascade_splits_[index] = n / (n - f) -
		                         (f * n) / ((n - f) * c);
	}
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
	for (uint32_t i = 0; i < cascade_count_; ++i)
	{
		glm::mat4              inverse_view_projection = glm::inverse(camera_.get_projection() * camera_.get_view());
		std::vector<glm::vec3> corners(8);
		for (uint32_t i = 0; i < 8; i++)
		{
			glm::vec4 homogenous_corner = glm::vec4(
			    (i & 1) ? 1.0f : -1.0f,
			    (i & 2) ? 1.0f : -1.0f,
			    (i & 4) ? cascade_splits_[i] : cascade_splits_[i + 1],
			    //(i & 4) ? 1.0f : 0.0f,
			    1.0f);

			glm::vec4 world_corner = inverse_view_projection * homogenous_corner;
			corners[i]             = glm::vec3(world_corner) / world_corner.w;
		}

		glm::mat4 light_view_mat = cascade_camera_->get_view();

		for (auto &corner : corners)
		{
			corner = light_view_mat * glm::vec4(corner, 1.0f);
		}

		glm::vec3 min_bounds = glm::vec3(FLT_MAX), max_bounds(FLT_MIN);

		for (auto &corner : corners)
		{
			// In vulkan, clip space has inverted Y and depth range, so we need to flip the Y and Z axis
			corner.y   = -corner.y;
			corner.z   = -corner.z;
			min_bounds = glm::min(min_bounds, corner);
			max_bounds = glm::max(max_bounds, corner);
		}

		cascade_camera_->set_bounds(min_bounds, max_bounds);
	}

	GlobalUniform global_uniform{};

	global_uniform.camera_view_proj = cascade_camera_->get_pre_rotation() * xihe::vulkan_style_projection(cascade_camera_->get_projection()) * cascade_camera_->get_view();

	auto &render_frame = render_context_.get_active_frame();

	auto &transform = node.get_transform();

	auto allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(GlobalUniform), thread_index);

	global_uniform.model           = transform.get_world_matrix();
	global_uniform.camera_position = glm::vec3((glm::inverse(cascade_camera_->get_view())[3]));

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
}

void ShadowSubpass::draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, const std::vector<backend::ShaderResource> &vertex_input_resources)
{
	VertexInputState vertex_input_state{};

	for (auto &input_resource : vertex_input_resources)
	{
		sg::VertexAttribute attribute;

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

	if (sub_mesh.vertex_indices != 0)
	{
		command_buffer.bind_index_buffer(*sub_mesh.index_buffer, sub_mesh.index_offset, sub_mesh.index_type);

		command_buffer.draw_indexed(sub_mesh.vertex_indices, 1, 0, 0, 0);
	}
	else
	{
		command_buffer.draw(sub_mesh.vertices_count, 1, 0, 0);
	}
}

void ShadowSubpass::prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face)
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
