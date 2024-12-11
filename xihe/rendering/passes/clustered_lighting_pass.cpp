#include "clustered_lighting_pass.h"

namespace xihe::rendering
{
void ClusteredLightingPass::sort_lights()
{
	for (uint32_t i = 0; i<lights_.size(); ++i)
	{
		sg::Light *light = lights_[i];
		if (light->get_light_type()!=sg::kPoint)
		{
			continue;
		}
		auto camera_view_proj = camera_.get_pre_rotation() * vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

		glm::vec4 light_pos = glm::vec4(light->get_node()->get_transform().get_translation(), 1.0f);

		glm::vec4 projected_pos = camera_view_proj * light_pos;
		glm::vec4 projected_pos_min = projected_pos - glm::vec4(0.0f, 0.0f, light->get_properties().range, 0.0f);
		glm::vec4 projected_pos_max = projected_pos + glm::vec4(0.0f, 0.0f, light->get_properties().range, 0.0f);

		SortedLight sorted_light;

		sorted_light.light_index = i;

		sorted_light.projected_z = (projected_pos.z - camera_.get_near_plane()) / (camera_.get_far_plane() - camera_.get_near_plane());
		sorted_light.projected_min = (projected_pos_min.z - camera_.get_near_plane()) / (camera_.get_far_plane() - camera_.get_near_plane());
		sorted_light.projected_max = (projected_pos_max.z - camera_.get_near_plane()) / (camera_.get_far_plane() - camera_.get_near_plane());
	}


}
}
