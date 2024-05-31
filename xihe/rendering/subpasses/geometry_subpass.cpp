#include "geometry_subpass.h"

#include "rendering/render_frame.h"
#include "scene_graph/scene.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/image.h"
#include "scene_graph/node.h"

namespace xihe::rendering
{
GeometrySubpass::GeometrySubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera) :
    Subpass{render_context, std::move(vertex_shader), std::move(fragment_shader)},
    camera_{camera},
    meshes_{scene_.get_components<sg::Mesh>()},
	scene_{scene}
{}

void GeometrySubpass::prepare()
{
	auto &device = render_context_.get_device();
	for (auto &mesh : meshes_)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant = sub_mesh->get_shader_variant();
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
		backend::ScopedDebugLabel label{command_buffer , "Opaque objects"};

		for (auto &node: opaque_nodes)
		{
			update_uniform(command_buffer, *node.second.first, thread_index_);
		}
	}
}

void GeometrySubpass::set_thread_index(uint32_t index)
{}

void GeometrySubpass::update_uniform(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index)
{
	GlobalUniform global_uniform{};

	global_uniform.camera_view_proj = camera_.get_pre_rotation() * xihe::vulkan_style_projection(camera_.get_projection() * camera_.get_view());

	auto &render_frame = render_context_.get_active_frame();

	auto &transform = node.get_transform();

	auto allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(GlobalUniform), thread_index);

	global_uniform.model = transform.get_world_matrix();
	global_uniform.camera_position = glm::vec3((glm::inverse(camera_.get_view())[3]));

	allocation.update(global_uniform);

	command_buffer.bind_vertex_buffers()
}

void GeometrySubpass::draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, vk::FrontFace front_face)
{}

void GeometrySubpass::prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face, bool double_sided_material)
{}

backend::PipelineLayout & GeometrySubpass::prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules)
{}

void GeometrySubpass::prepare_push_constants(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh)
{}

void GeometrySubpass::draw_submesh_command(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh)
{}

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
}
