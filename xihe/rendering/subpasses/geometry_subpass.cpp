#include "geometry_subpass.h"

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

#include <ranges>

namespace xihe::rendering
{
GeometrySubpass::GeometrySubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera) :
    Subpass{render_context, std::move(vertex_shader), std::move(fragment_shader)},
    camera_{camera},
    meshes_{scene.get_components<sg::Mesh>()},
    scene_{scene}
{}

void GeometrySubpass::prepare()
{
	auto &device = render_context_.get_device();
	for (auto &mesh : meshes_)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant     = sub_mesh->get_shader_variant();
			auto &vert_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), variant);
			auto &frag_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), variant);
		}
	}
}

void GeometrySubpass::draw(backend::CommandBuffer &command_buffer)
{
	std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> opaque_nodes;
	std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> transparent_nodes;

	get_sorted_nodes(opaque_nodes, transparent_nodes);

	// Draw opaque objects in front-to-back order
	{
		backend::ScopedDebugLabel label{command_buffer, "Opaque objects"};

		for (const auto &[node, sub_mesh] : opaque_nodes | std::views::values)
		{
			update_uniform(command_buffer, *node, thread_index_);

			draw_submesh(command_buffer, *sub_mesh);
		}
	}

	ColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blend_enable           = true;
	color_blend_attachment.src_color_blend_factor = vk::BlendFactor::eSrcAlpha;
	color_blend_attachment.dst_color_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;
	color_blend_attachment.src_alpha_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;

	ColorBlendState color_blend_state{};
	color_blend_state.attachments.resize(get_output_attachments().size());
	for (auto &it : color_blend_state.attachments)
	{
		it = color_blend_attachment;
	}

	command_buffer.set_color_blend_state(color_blend_state);
	
	command_buffer.set_depth_stencil_state(get_depth_stencil_state());

	// Draw transparent objects in back-to-front order
	{
		backend::ScopedDebugLabel transprent_debug_label{command_buffer, "Transparent objects"};

		for (const auto &[node, sub_mesh] : transparent_nodes | std::views::values | std::views::reverse)
		{
			update_uniform(command_buffer, *node, thread_index_);

			draw_submesh(command_buffer, *sub_mesh);
		}
	}
}

void GeometrySubpass::update_uniform(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index)
{
	SceneUniform global_uniform{};

	global_uniform.camera_view_proj = camera_.get_pre_rotation() * xihe::vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

	auto &render_frame = render_context_.get_active_frame();

	auto &transform = node.get_transform();

	auto allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SceneUniform), thread_index);

	global_uniform.model           = transform.get_world_matrix();
	global_uniform.camera_position = glm::vec3((glm::inverse(camera_.get_view())[3]));

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
}

void GeometrySubpass::draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, vk::FrontFace front_face)
{
	auto &device = command_buffer.get_device();

	backend::ScopedDebugLabel{command_buffer, sub_mesh.get_name().c_str()};

	prepare_pipeline_state(command_buffer, front_face, sub_mesh.get_material()->double_sided);

	auto &vert_shader_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), sub_mesh.get_shader_variant());
	auto &frag_shader_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), sub_mesh.get_shader_variant());

	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = prepare_pipeline_layout(command_buffer, shader_modules);

	command_buffer.bind_pipeline_layout(pipeline_layout);

	if (pipeline_layout.get_push_constant_range_stage(sizeof(PBRMaterialUniform)))
	{
		prepare_push_constants(command_buffer, sub_mesh);
	}

	const backend::DescriptorSetLayout &descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(0);

	for (auto &[name, texture] : sub_mesh.get_material()->textures)
	{
		if (const auto layout_binding = descriptor_set_layout.get_layout_binding(name))
		{
			command_buffer.bind_image(texture->get_image()->get_vk_image_view(),
			                          texture->get_sampler()->vk_sampler_,
			                          0, layout_binding->binding, 0);
		}
	}

	auto vertex_input_resources = pipeline_layout.get_resources(backend::ShaderResourceType::kInput, vk::ShaderStageFlagBits::eVertex);

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

	draw_submesh_command(command_buffer, sub_mesh);
}

void GeometrySubpass::prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face, bool double_sided_material)
{
	RasterizationState rasterization_state = base_rasterization_state_;
	rasterization_state.front_face         = front_face;

	if (double_sided_material)
	{
		rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	}

	command_buffer.set_rasterization_state(rasterization_state);

	MultisampleState multisample_state{};
	multisample_state.rasterization_samples = sample_count_;
	command_buffer.set_multisample_state(multisample_state);
}

backend::PipelineLayout &GeometrySubpass::prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules)
{
	for (auto &shader_module : shader_modules)
	{
		for (auto &[name, mode] : resource_mode_map_)
		{
			shader_module->set_resource_mode(name, mode);
		}
	}
	return command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules, render_context_.get_bindless_descriptor_set());
}

void GeometrySubpass::prepare_push_constants(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh)
{
	const auto pbr_material = dynamic_cast<const sg::PbrMaterial *>(sub_mesh.get_material());

	PBRMaterialUniform pbr_material_uniform;
	pbr_material_uniform.texture_indices   = pbr_material->texture_indices;
	pbr_material_uniform.base_color_factor = pbr_material->base_color_factor;
	pbr_material_uniform.metallic_factor   = pbr_material->metallic_factor;
	pbr_material_uniform.roughness_factor  = pbr_material->roughness_factor;

	if (const auto data = to_bytes(pbr_material_uniform); !data.empty())
	{
		command_buffer.push_constants(data);
	}
}

void GeometrySubpass::draw_submesh_command(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh)
{
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

void GeometrySubpass::get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &opaque_nodes, std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &transparent_nodes) const
{
	auto camera_transform = camera_.get_node()->get_transform().get_world_matrix();

	for (auto &mesh : meshes_)
	{
		for (auto &node : mesh->get_nodes())
		{
			auto node_transform = node->get_transform().get_world_matrix();

			const sg::AABB &mesh_bounds = mesh->get_bounds();

			sg::AABB world_bounds{mesh_bounds.get_min(), mesh_bounds.get_max()};
			world_bounds.transform(node_transform);

			float distance = glm::length(glm::vec3(camera_transform[3]) - world_bounds.get_center());

			for (auto &sub_mesh : mesh->get_submeshes())
			{
				if (sub_mesh->get_material()->alpha_mode == sg::AlphaMode::kBlend)
				{
					transparent_nodes.emplace(distance, std::make_pair(node, sub_mesh));
				}
				else
				{
					opaque_nodes.emplace(distance, std::make_pair(node, sub_mesh));
				}
			}
		}
	}
}
}        // namespace xihe::rendering
