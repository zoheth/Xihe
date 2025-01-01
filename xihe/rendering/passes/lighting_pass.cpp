#include "lighting_pass.h"

namespace xihe::rendering
{

vk::SamplerCreateInfo get_g_buffer_sampler()
{
	return {};
}

vk::SamplerCreateInfo get_shadowmap_sampler()
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
	return shadowmap_sampler_create_info;
}

void bind_lighting(backend::CommandBuffer &command_buffer, const LightingState &lighting_state, uint32_t set, uint32_t binding)
{
	command_buffer.bind_buffer(lighting_state.light_buffer.get_buffer(),
	                           lighting_state.light_buffer.get_offset(),
	                           lighting_state.light_buffer.get_size(),
	                           set, binding, 0);
	command_buffer.set_specialization_constant(0, to_u32(lighting_state.directional_lights.size()));
	command_buffer.set_specialization_constant(1, to_u32(lighting_state.point_lights.size()));
	command_buffer.set_specialization_constant(2, to_u32(lighting_state.spot_lights.size()));
}

LightingPass::LightingPass(std::vector<sg::Light *> lights, sg::Camera &camera, sg::CascadeScript *cascade_script) :
    lights_{std::move(lights)}, camera_(camera), cascade_script_{cascade_script}
{
	shader_variant_.add_define({"MAX_LIGHT_COUNT " + std::to_string(kMaxLightCount)});
	shader_variant_cascade_.add_define({"MAX_LIGHT_COUNT " + std::to_string(kMaxLightCount)});

	shader_variant_.add_definitions(kLightTypeDefinitions);
	shader_variant_cascade_.add_definitions(kLightTypeDefinitions);

	shader_variant_cascade_.add_define("SHOW_CASCADE_VIEW");
}

void LightingPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	set_lighting_state(kMaxLightCount);
	set_pipeline_state(command_buffer);

	DeferredLights light_info;

	std::ranges::copy(lighting_state_.directional_lights, light_info.directional_lights);
	std::ranges::copy(lighting_state_.point_lights, light_info.point_lights);
	std::ranges::copy(lighting_state_.spot_lights, light_info.spot_lights);

	lighting_state_.light_buffer = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(DeferredLights), thread_index_);
	lighting_state_.light_buffer.update(light_info);

	bind_lighting(command_buffer, lighting_state_, 0, 4);

	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	

	command_buffer.bind_image(input_bindables[0].image_view(), resource_cache.request_sampler(get_g_buffer_sampler()), 0, 0, 0);
	command_buffer.bind_image(input_bindables[1].image_view(), resource_cache.request_sampler(get_g_buffer_sampler()), 0, 1, 0);
	command_buffer.bind_image(input_bindables[2].image_view(), resource_cache.request_sampler(get_g_buffer_sampler()), 0, 2, 0);

	LightUniform light_uniform;

	light_uniform.inv_view_proj = glm::inverse(vulkan_style_projection(camera_.get_projection()) * camera_.get_view());

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(LightUniform), thread_index_);
	allocation.update(light_uniform);
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 3, 0);

	if (cascade_script_)
	{
		ShadowUniform shadow_uniform{};

		for (uint32_t i = 0; i < kCascadeCount; ++i)
		{
			auto &cascade_camera                          = cascade_script_->get_cascade_camera(i);
			shadow_uniform.cascade_split_depth[i]         = cascade_script_->get_cascade_splits()[i];
			shadow_uniform.shadowmap_projection_matrix[i] = vulkan_style_projection(cascade_camera.get_projection()) * cascade_camera.get_view();
		}

		allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(ShadowUniform), thread_index_);
		allocation.update(shadow_uniform);
		command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 5, 0);

		command_buffer.bind_image(input_bindables[3].image_view(), resource_cache.request_sampler(get_shadowmap_sampler()), 0, 6, 0);
	}

	if (input_bindables.size() > 4)
	{
		command_buffer.bind_image(input_bindables[4].image_view(), resource_cache.request_sampler(get_shadowmap_sampler()), 0, 10, 0);
	}

	command_buffer.draw(3, 1, 0, 0);
}

void LightingPass::show_cascade_view(bool show)
{
	show_cascade_view_ = show;
}

void LightingPass::set_lighting_state(size_t light_count)
{
	assert(lights_.size() <= (light_count * sg::LightType::kMax) && "Exceeding Max Light Capacity");

	lighting_state_.directional_lights.clear();
	lighting_state_.point_lights.clear();
	lighting_state_.spot_lights.clear();

	for (auto &scene_light : lights_)
	{
		const auto &properties = scene_light->get_properties();
		auto       &transform  = scene_light->get_node()->get_transform();

		Light light{{transform.get_translation(), static_cast<float>(scene_light->get_light_type())},
		            {properties.color, properties.intensity},
		            {transform.get_rotation() * properties.direction, properties.range},
		            {properties.inner_cone_angle, properties.outer_cone_angle}};

		switch (scene_light->get_light_type())
		{
			case sg::LightType::kDirectional:
			{
				if (lighting_state_.directional_lights.size() < light_count)
				{
					lighting_state_.directional_lights.push_back(light);
				}
				break;
			}
			case sg::LightType::kPoint:
			{
				if (lighting_state_.point_lights.size() < light_count)
				{
					lighting_state_.point_lights.push_back(light);
				}
				break;
			}
			case sg::LightType::kSpot:
			{
				if (lighting_state_.spot_lights.size() < light_count)
				{
					lighting_state_.spot_lights.push_back(light);
				}
				break;
			}
			default:
				break;
		}
	}
}

void LightingPass::set_pipeline_state(backend::CommandBuffer &command_buffer)
{
	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	std::vector<backend::ShaderModule *> shader_modules;
	if (show_cascade_view_)
	{
		auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), shader_variant_cascade_);
		auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), shader_variant_cascade_);
		shader_modules           = {&vert_shader_module, &frag_shader_module};
	}
	else
	{
		auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), shader_variant_);
		auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), shader_variant_);
		shader_modules           = {&vert_shader_module, &frag_shader_module};
	}

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules, &resource_cache.request_bindless_descriptor_set());
	command_buffer.bind_pipeline_layout(pipeline_layout);

	// we know, that the lighting subpass does not have any vertex stage input -> reset the vertex input state
	assert(pipeline_layout.get_resources(backend::ShaderResourceType::kInput, vk::ShaderStageFlagBits::eVertex).empty());
	command_buffer.set_vertex_input_state({});

	RasterizationState rasterization_state;
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);
}

}        // namespace xihe::rendering