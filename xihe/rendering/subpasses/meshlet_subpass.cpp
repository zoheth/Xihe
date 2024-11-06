#include "meshlet_subpass.h"

#include "forward_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/mesh.h"
#include "rendering/render_context.h"
#include "scene_graph/scene.h"
#include "shared_uniform.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/image.h"

#include <ranges>

constexpr uint32_t kMaxForwardLightCount = 4;

namespace xihe::rendering
{
MeshletSubpass::MeshletSubpass(rendering::RenderContext &render_context, std::optional<backend::ShaderSource> task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera) :
    Subpass{render_context, std::move(task_shader), std::move(mesh_shader), std::move(fragment_shader)},
	camera_{camera},
	scene_{scene},
    meshes_{scene.get_components<sg::Mesh>()}
{
	/*mesh_         = scene.get_components<sg::Mesh>()[0];
	mshader_mesh_ = mesh_->get_mshader_meshes()[0];*/
	// assert(mshader_mesh_);
}

void MeshletSubpass::prepare()
{
	auto &device  = get_render_context().get_device();

	for (auto &mesh : meshes_)
	{
		for (auto &mshader_mesh : mesh->get_mshader_meshes())
		{
			auto &variant = mshader_mesh->get_mut_shader_variant();
			//variant.add_definitions({"MAX_LIGHT_COUNT " + std::to_string(kMaxForwardLightCount)});
			//variant.add_definitions(kLightTypeDefinitions);
			auto &mesh_shader_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader(), variant);
			auto &frag_shader_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), variant);
		}
	}
}
void MeshletSubpass::draw(backend::CommandBuffer &command_buffer)
{
	std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> opaque_nodes;
	std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> transparent_nodes;

	get_sorted_nodes(opaque_nodes, transparent_nodes);

	command_buffer.set_has_mesh_shader(true);

	{
		backend::ScopedDebugLabel label{command_buffer, "Opaque objects"};

		for (const auto &[node, mshader_mesh] : opaque_nodes | std::views::values)
		{
			update_uniform(command_buffer, *node, thread_index_);
			draw_mshader_mesh(command_buffer, *mshader_mesh);
		}
	}

	command_buffer.set_has_mesh_shader(false);
}
void MeshletSubpass::update_uniform(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index) const
{
	SceneUniform global_uniform{};
	global_uniform.camera_view_proj = camera_.get_pre_rotation() * xihe::vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

	global_uniform.camera_position = glm::vec3((glm::inverse(camera_.get_view())[3]));

	global_uniform.model = node.get_transform().get_world_matrix();

	auto &render_frame = render_context_.get_active_frame();
	auto  allocation   = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SceneUniform), thread_index);

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 2, 0);
}

backend::PipelineLayout & MeshletSubpass::prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules)
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

void MeshletSubpass::draw_mshader_mesh(backend::CommandBuffer &command_buffer, sg::MshaderMesh &mshader_mesh)
{
	auto &mesh_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader(), mshader_mesh.get_shader_variant());
	auto &frag_shader_module = command_buffer.get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), mshader_mesh.get_shader_variant());

	std::vector<backend::ShaderModule *> shader_modules{&mesh_shader_module, &frag_shader_module};

	auto &pipeline_layout = prepare_pipeline_layout(command_buffer, shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	if (pipeline_layout.get_push_constant_range_stage(sizeof(PBRMaterialUniform)))
	{
		const auto pbr_material = dynamic_cast<const sg::PbrMaterial *>(mshader_mesh.get_material());

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

	const backend::DescriptorSetLayout &descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(0);

	for (auto &[name, texture] : mshader_mesh.get_material()->textures)
	{
		if (const auto layout_binding = descriptor_set_layout.get_layout_binding(name))
		{
			command_buffer.bind_image(texture->get_image()->get_vk_image_view(),
			                          texture->get_sampler()->vk_sampler_,
			                          0, layout_binding->binding, 0);
		}
	}

	command_buffer.bind_buffer(mshader_mesh.get_meshlet_buffer(), 0, mshader_mesh.get_meshlet_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(mshader_mesh.get_vertex_buffer(), 0, mshader_mesh.get_vertex_buffer().get_size(), 0, 4, 0);
	command_buffer.bind_buffer(mshader_mesh.get_meshlet_vertices_buffer(), 0, mshader_mesh.get_meshlet_vertices_buffer().get_size(), 0, 5, 0);
	command_buffer.bind_buffer(mshader_mesh.get_packed_meshlet_indices_buffer(), 0, mshader_mesh.get_packed_meshlet_indices_buffer().get_size(), 0, 6, 0);

	uint32_t num_workgroups_x = mshader_mesh.get_meshlet_count();        // meshlets count
	uint32_t num_workgroups_y = 1;
	uint32_t num_workgroups_z = 1;
	command_buffer.draw_mesh_tasks(num_workgroups_x, num_workgroups_y, num_workgroups_z);
}

void MeshletSubpass::get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &opaque_nodes, std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &transparent_nodes) const
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

			for (auto &mshader_mesh_mesh : mesh->get_mshader_meshes())
			{
				// todo  handle transparent objects
				opaque_nodes.emplace(distance, std::make_pair(node, mshader_mesh_mesh));
				
			}
		}
	}
}
}        // namespace xihe::rendering