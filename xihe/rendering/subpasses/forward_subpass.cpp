#include "forward_subpass.h"

#include "rendering/render_context.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"
#include "scene_graph/components/light.h"

namespace xihe::rendering
{
ForwardSubpass::ForwardSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_source, backend::ShaderSource &&fragment_source, sg::Scene &scene, sg::Camera &camera) :
    GeometrySubpass{render_context, std::move(vertex_source), std::move(fragment_source), scene, camera}
{
}

void ForwardSubpass::prepare()
{
	auto &device = render_context_.get_device();
	for (auto &mesh : meshes_)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant = sub_mesh->get_mut_shader_variant();

			// Same as Geometry except adds lighting definitions to sub mesh variants.
			variant.add_definitions({"MAX_LIGHT_COUNT " + std::to_string(MAX_FORWARD_LIGHT_COUNT)});

			variant.add_definitions(kLightTypeDefinitions);

			auto &vert_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), variant);
			auto &frag_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), variant);
		}
	}
}

void ForwardSubpass::draw(backend::CommandBuffer &command_buffer)
{
	allocate_lights<ForwardLights>(scene_.get_components<sg::Light>(), MAX_FORWARD_LIGHT_COUNT);
	command_buffer.bind_lighting(get_lighting_state(), 0, 4);

	GeometrySubpass::draw(command_buffer);
}
}
