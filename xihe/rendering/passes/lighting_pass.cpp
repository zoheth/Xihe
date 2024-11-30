#include "lighting_pass.h"

namespace xihe::rendering
{
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

LightingPass::LightingPass(std::vector<sg::Light *> lights) :
    lights_{std::move(lights)}
{
	shader_variant_.add_define({"MAX_LIGHT_COUNT " + std::to_string(kMaxLightCount)});
}

void LightingPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame)
{
	set_lighting_state(kMaxLightCount);

	DeferredLights light_info;

	std::ranges::copy(lighting_state_.directional_lights, light_info.directional_lights);
	std::ranges::copy(lighting_state_.point_lights, light_info.point_lights);
	std::ranges::copy(lighting_state_.spot_lights, light_info.spot_lights);

	lighting_state_.light_buffer = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(DeferredLights), 0);
	lighting_state_.light_buffer.update(light_info);

	bind_lighting(command_buffer, lighting_state_, 0, 4);

	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), shader_variant_);
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), shader_variant_);

	std::vector<backend::ShaderModule *> shader_modules = {&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules, &resource_cache.request_bindless_descriptor_set());
	command_buffer.bind_pipeline_layout(pipeline_layout);


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
}        // namespace xihe::rendering