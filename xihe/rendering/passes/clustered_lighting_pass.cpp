#include "clustered_lighting_pass.h"

#include <algorithm>
#include <utility>

namespace xihe::rendering
{
namespace
{

glm::vec3 project_to_ndc(const glm::mat4 &projection_matrix, const glm::vec3 &camera_space_point)
{
	glm::vec4 projected = projection_matrix * glm::vec4(camera_space_point, 1.0f);
	return glm::vec3(projected) / projected.w;
}

// Implementation of McGuire's method for computing sphere bounds along an axis
void get_axis_bounds(const glm::vec3 &axis,                 // Camera space axis
                     const glm::vec3 &sphere_center,        // Camera space sphere center
                     float            sphere_radius,        // Sphere radius
                     float            near_z,               // Near plane distance (negative)
                     glm::vec3       &min_bound,            // Output min bound point
                     glm::vec3       &max_bound)
{        // Output max bound point

	glm::vec2 center_proj(glm::dot(axis, sphere_center), sphere_center.z);
	glm::vec2 bounds[2];

	float t_squared        = glm::dot(center_proj, center_proj) - (sphere_radius * sphere_radius);
	bool  is_camera_inside = (t_squared <= 0);

	glm::vec2 tangent_angles;
	if (is_camera_inside)
	{
		tangent_angles = glm::vec2(0.0f);
	}
	else
	{
		float norm     = glm::length(center_proj);
		tangent_angles = glm::vec2(sqrt(t_squared), sphere_radius) / norm;
	}

	bool  sphere_intersects_near = (center_proj.y + sphere_radius >= near_z);
	float discriminant           = sqrt(glm::max(0.0f,
	                                             sphere_radius * sphere_radius -
	                                                 (near_z - center_proj.y) * (near_z - center_proj.y)));

	for (auto &bound : bounds)
	{
		if (!is_camera_inside)
		{
			glm::mat2 transform(
			    tangent_angles.x, -tangent_angles.y,
			    tangent_angles.y, tangent_angles.x);
			bound = transform * (center_proj * tangent_angles.x);
		}

		bool bound_needs_clip = is_camera_inside || (bound.y > near_z);
		if (sphere_intersects_near && bound_needs_clip)
		{
			bound = glm::vec2(center_proj.x + discriminant, near_z);
		}

		tangent_angles.y = -tangent_angles.y;
		discriminant     = -discriminant;
	}

	min_bound   = axis * bounds[1].x;
	min_bound.z = bounds[1].y;
	max_bound   = axis * bounds[0].x;
	max_bound.z = bounds[0].y;
}
}        // namespace

ClusteredLightingPass::ClusteredLightingPass(std::vector<sg::Light *> lights, sg::Camera &camera, sg::CascadeScript *cascade_script) :
    LightingPass(std::move(lights), camera, cascade_script)
{
}

void ClusteredLightingPass::generate_lighting_data()
{
	collect_and_sort_lights();
	generate_bins();
	generate_tiles();
}

void ClusteredLightingPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	width_  = input_bindables[0].image_view().get_image().get_extent().width;
	height_ = input_bindables[0].image_view().get_image().get_extent().height;
	generate_lighting_data();

	set_lighting_state(kMaxPointLightCount);
	set_pipeline_state(command_buffer);

	ClusteredLights light_info;

	std::ranges::copy(lighting_state_.directional_lights, light_info.directional_lights);
	std::ranges::copy(lighting_state_.point_lights, light_info.point_lights);
	std::ranges::copy(lighting_state_.spot_lights, light_info.spot_lights);

	lighting_state_.light_buffer = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(ClusteredLights), thread_index_);
	lighting_state_.light_buffer.update(light_info);

	bind_lighting(command_buffer, lighting_state_, 0, 4);

	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	command_buffer.bind_image(input_bindables[0].image_view(), resource_cache.request_sampler(get_g_buffer_sampler()), 0, 0, 0);
	command_buffer.bind_image(input_bindables[1].image_view(), resource_cache.request_sampler(get_g_buffer_sampler()), 0, 1, 0);
	command_buffer.bind_image(input_bindables[2].image_view(), resource_cache.request_sampler(get_g_buffer_sampler()), 0, 2, 0);

	ClusteredLightUniform light_uniform;

	light_uniform.inv_view_proj = glm::inverse(vulkan_style_projection(camera_.get_projection()) * camera_.get_view());
	light_uniform.view          = camera_.get_view();
	light_uniform.near_plane    = camera_.get_near_plane();
	light_uniform.far_plane     = camera_.get_far_plane();

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(ClusteredLightUniform), thread_index_);
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

	{
		auto storage_allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eStorageBuffer, bins_.size() * 4, thread_index_);
		storage_allocation.update(bins_);
		command_buffer.bind_buffer(storage_allocation.get_buffer(), storage_allocation.get_offset(), storage_allocation.get_size(), 0, 7, 0);
	}
	{
		auto storage_allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eStorageBuffer, tiles_.size() * 4, thread_index_);
		storage_allocation.update(tiles_);
		command_buffer.bind_buffer(storage_allocation.get_buffer(), storage_allocation.get_offset(), storage_allocation.get_size(), 0, 8, 0);
	}
	{
		auto storage_allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eStorageBuffer, light_indices_.size() * 4, thread_index_);
		storage_allocation.update(light_indices_);
		command_buffer.bind_buffer(storage_allocation.get_buffer(), storage_allocation.get_offset(), storage_allocation.get_size(), 0, 9, 0);
	}

	command_buffer.draw(3, 1, 0, 0);
}

void ClusteredLightingPass::collect_and_sort_lights()
{
	sorted_lights_.clear();
	auto camera_view_mat = camera_.get_view();

	for (uint32_t i = 0; i < lights_.size(); ++i)
	{
		sg::Light *light = lights_[i];
		if (light->get_light_type() != sg::kPoint)
		{
			continue;
		}

		glm::vec4 light_pos         = glm::vec4(light->get_node()->get_transform().get_translation(), 1.0f);
		glm::vec4 projected_pos     = camera_view_mat * light_pos;
		float     range             = light->get_properties().range;
		projected_pos.z             = projected_pos.z;
		glm::vec4 projected_pos_min = projected_pos + glm::vec4(0.0f, 0.0f, range, 0.0f);
		glm::vec4 projected_pos_max = projected_pos - glm::vec4(0.0f, 0.0f, range, 0.0f);

		SortedLight sorted_light;
		sorted_light.light_index = i;
		sorted_light.projected_z = (-projected_pos.z - camera_.get_near_plane()) /
		                           (camera_.get_far_plane() - camera_.get_near_plane());
		sorted_light.projected_z_min = (-projected_pos_min.z - camera_.get_near_plane()) /
		                               (camera_.get_far_plane() - camera_.get_near_plane());
		sorted_light.projected_z_max = (-projected_pos_max.z - camera_.get_near_plane()) /
		                               (camera_.get_far_plane() - camera_.get_near_plane());

		sorted_lights_.push_back(sorted_light);
	}

	std::ranges::sort(sorted_lights_,
	                  [](const SortedLight &a, const SortedLight &b) {
		                  return a.projected_z < b.projected_z;
	                  });

	light_indices_.resize(sorted_lights_.size());
	for (size_t i = 0; i < sorted_lights_.size(); ++i)
	{
		light_indices_[i] = sorted_lights_[i].light_index;
	}
}

void ClusteredLightingPass::generate_bins()
{
	bins_.resize(num_bins_, 0xFFFF0000);        // initialize to invalid value (max=0xFFFF, min=0)

	constexpr float bin_width = 1.0f / num_bins_;

	std::vector<uint32_t> bin_range_per_light(sorted_lights_.size(), 0xffffffffui32);

	for (uint32_t i = 0; i < sorted_lights_.size(); ++i)
	{
		const SortedLight &light = sorted_lights_[i];
		if (light.projected_z_min < 0.0f && light.projected_z_max < 0.0f)
		{
			continue;
		}
		const uint32_t min_bin = std::max(0, static_cast<int>(std::floor(light.projected_z_min / bin_width)));
		const uint32_t max_bin = std::max(0, static_cast<int>(std::ceil(light.projected_z_max / bin_width)));

		bin_range_per_light[i] = (min_bin & 0xffff) | ((max_bin & 0xffff) << 16);
	}

	for (uint32_t bin = 0; bin < num_bins_; ++bin)
	{
		uint32_t min_light_id = num_lights_ + 1;
		uint32_t max_light_id = 0;

		float bin_min = bin * bin_width;
		float bin_max = bin_min + bin_width;

		for (uint32_t i = 0; i < sorted_lights_.size(); ++i)
		{
			const SortedLight &light      = sorted_lights_[i];
			const uint32_t     light_bins = bin_range_per_light[i];

			if (light_bins == 0xffffffff)
			{
				continue;
			}

			const uint32_t min_bin = light_bins & 0xffff;
			const uint32_t max_bin = light_bins >> 16;

			if (bin >= min_bin && bin <= max_bin)
			{
				if (i < min_light_id)
				{
					min_light_id = i;
				}

				if (i > max_light_id)
				{
					max_light_id = i;
				}
			}
		}
		bins_[bin] = min_light_id | (max_light_id << 16);
	}
}

void ClusteredLightingPass::generate_tiles()
{
	num_tiles_x_ = width_ / tile_size_;
	num_tiles_y_ = height_ / tile_size_;

	tiles_.resize(num_tiles_x_ * num_tiles_y_ * num_words_, 0);

	float tile_size_inv = 1.0f / tile_size_;

	uint32_t tile_stride = num_tiles_x_ * num_words_;

	auto camera_view = camera_.get_view();

	for (size_t sorted_idx = 0; sorted_idx < sorted_lights_.size(); ++sorted_idx)
	{
		const auto &light          = lights_[sorted_lights_[sorted_idx].light_index];
		glm::vec4   light_pos      = glm::vec4(light->get_node()->get_transform().get_translation(), 1.0f);
		glm::vec4   view_space_pos = camera_view * light_pos;
		float       range          = light->get_properties().range;

		if (-view_space_pos.z + range < camera_.get_near_plane())
		{
			continue;
		}

		glm::vec3 left, right, bottom, top;
		get_axis_bounds(glm::vec3(1.0f, 0.0f, 0.0f), view_space_pos, range, camera_.get_near_plane(), left, right);
		get_axis_bounds(glm::vec3(0.0f, 1.0f, 0.0f), view_space_pos, range, camera_.get_near_plane(), top, bottom);

		sg::PerspectiveCamera *perspective_camera = dynamic_cast<sg::PerspectiveCamera *>(&camera_);

		auto projection_mat = glm::perspective(perspective_camera->get_field_of_view(), perspective_camera->get_aspect_ratio(), perspective_camera->get_near_plane(), perspective_camera->get_far_plane());

		glm::vec3 proj_left   = project_to_ndc(projection_mat, left);
		glm::vec3 proj_right  = project_to_ndc(projection_mat, right);
		glm::vec3 proj_top    = project_to_ndc(projection_mat, top);
		glm::vec3 proj_bottom = project_to_ndc(projection_mat, bottom);

		auto aabb = glm::vec4(
		    proj_right.x,         // min x
		    -proj_top.y,          // min y
		    proj_left.x,          // max x
		    -proj_bottom.y        // max y
		);

		const float position_len = glm::length(glm::vec3(view_space_pos));
		if(position_len - range < camera_.get_near_plane())
		{
			aabb = glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f);
		}

		glm::vec4 aabb_screen{
		    (aabb.x * 0.5f + 0.5f) * (width_ - 1),
		    (aabb.y * 0.5f + 0.5f) * (height_ - 1),
		    (aabb.z * 0.5f + 0.5f) * (width_ - 1),
		    (aabb.w * 0.5f + 0.5f) * (height_ - 1)};

		float width  = aabb_screen.z - aabb_screen.x;
		float height = aabb_screen.w - aabb_screen.y;

		if (width < 0.0001f || height < 0.0001f)
		{
			continue;
		}

		float min_x = aabb_screen.x;
		float min_y = aabb_screen.y;

		float max_x = min_x + width;
		float max_y = min_y + height;

		if (min_x > width_ || min_y > height_ || max_x < 0 || max_y < 0)
		{
			continue;
		}

		min_x = std::max(min_x, 0.0f);
		min_y = std::max(min_y, 0.0f);
		max_x = std::min(max_x, static_cast<float>(width_));
		max_y = std::min(max_y, static_cast<float>(height_));

		uint32_t first_tile_x = static_cast<uint32_t>(min_x * tile_size_inv);
		uint32_t first_tile_y = static_cast<uint32_t>(min_y * tile_size_inv);
		uint32_t last_tile_x  = std::min(static_cast<uint32_t>(max_x * tile_size_inv), num_tiles_x_ - 1);
		uint32_t last_tile_y  = std::min(static_cast<uint32_t>(max_y * tile_size_inv), num_tiles_y_ - 1);

		for (uint32_t y = first_tile_y; y <= last_tile_y; ++y)
		{
			for (uint32_t x = first_tile_x; x <= last_tile_x; ++x)
			{
				uint32_t array_index = y * tile_stride + x;

				uint32_t word_index = sorted_idx / 32;
				uint32_t bit_index  = sorted_idx % 32;

				tiles_[array_index + word_index] |= (1 << bit_index);
			}
		}
	}
}
}        // namespace xihe::rendering
