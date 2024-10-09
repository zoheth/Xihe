#include "lighting_subpass.h"

#include "rendering/render_context.h"
#include "rendering/subpasses/shadow_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/scene.h"
#include "scene_graph/scripts/cascade_script.h"

namespace xihe::rendering
{
LightingSubpass::LightingSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader, sg::Camera &camera, sg::Scene &scene, sg::CascadeScript *cascade_script_ptr) :
    Subpass{render_context, std::move(vertex_shader), std::move(fragment_shader)}, camera_{camera}, scene_{scene}
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

	shadowmap_sampler_ = std::make_unique<backend::Sampler>(render_context_.get_device(), shadowmap_sampler_create_info);

	cascade_script_ = cascade_script_ptr;
}

void LightingSubpass::prepare()
{
	shader_variant_.add_define({"MAX_LIGHT_COUNT " + std::to_string(MAX_DEFERRED_LIGHT_COUNT)});

	shader_variant_.add_definitions(kLightTypeDefinitions);

	auto &resource_cache = render_context_.get_device().get_resource_cache();

	resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), shader_variant_);
	resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), shader_variant_);
}

void LightingSubpass::draw(backend::CommandBuffer &command_buffer)
{
	allocate_lights<DeferredLights>(scene_.get_components<sg::Light>(), MAX_DEFERRED_LIGHT_COUNT);
	command_buffer.bind_lighting(get_lighting_state(), 0, 4);

	auto &resource_cache = render_context_.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), shader_variant_);
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), shader_variant_);

	std::vector shader_modules = {&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules, render_context_.get_bindless_descriptor_set());
	command_buffer.bind_pipeline_layout(pipeline_layout);

	// we know, that the lighting subpass does not have any vertex stage input -> reset the vertex input state
	assert(pipeline_layout.get_resources(backend::ShaderResourceType::kInput, vk::ShaderStageFlagBits::eVertex).empty());
	command_buffer.set_vertex_input_state({});

	assert(render_target_);
	auto &render_target = *render_target_;
	auto &target_views  = render_target.get_views();
	assert(3 < target_views.size());

	auto &depth_view = target_views[1];
	command_buffer.bind_input(depth_view, 0, 0, 0);

	auto &albedo_view = target_views[2];
	command_buffer.bind_input(albedo_view, 0, 1, 0);

	auto &normal_view = target_views[3];
	command_buffer.bind_input(normal_view, 0, 2, 0);

	RasterizationState rasterization_state;
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	LightUniform light_uniform;

	light_uniform.inv_resolution = {1.0f / render_target.get_extent().width, 1.0f / render_target.get_extent().height};

	light_uniform.inv_view_proj = glm::inverse(vulkan_style_projection(camera_.get_projection()) * camera_.get_view());

	auto &render_frame = get_render_context().get_active_frame();
	auto  allocation   = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(LightUniform), thread_index_);
	allocation.update(light_uniform);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 3, 0);

	// shadow
	ShadowUniform shadow_uniform{};

	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		auto &cascade_camera                          = cascade_script_->get_cascade_camera(i);
		shadow_uniform.cascade_split_depth[i]         = cascade_script_->get_cascade_splits()[i];
		shadow_uniform.shadowmap_projection_matrix[i] = vulkan_style_projection(cascade_camera.get_projection()) * cascade_camera.get_view();
	}
	const RenderTarget &shadow_render_target = render_frame.get_render_target("shadow_pass");
	shadow_uniform.shadowmap_first_index     = shadow_render_target.get_first_bindless_descriptor_set_index();
	allocation                               = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(ShadowUniform), thread_index_);
	allocation.update(shadow_uniform);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 5, 0);

	/*command_buffer.bind_image(shadow_render_target.get_views()[0], *shadowmap_sampler_, 0, 6, 0);
	command_buffer.bind_image(shadow_render_target.get_views()[1], *shadowmap_sampler_, 0, 7, 0);
	command_buffer.bind_image(shadow_render_target.get_views()[2], *shadowmap_sampler_, 0, 8, 0);*/

	command_buffer.draw(3, 1, 0, 0);
}
}        // namespace xihe::rendering
