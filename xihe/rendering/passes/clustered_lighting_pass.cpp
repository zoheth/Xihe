#include "clustered_lighting_pass.h"

#include <algorithm>
#include <utility>

namespace xihe::rendering
{
ClusteredLightingPass::ClusteredLightingPass(std::vector<sg::Light *> lights, sg::Camera &camera, backend::Device &device, uint32_t width, uint32_t height, sg::CascadeScript *cascade_script) :
    LightingPass(std::move(lights), camera, cascade_script), width_(width), height_(height), last_width_(width), last_height_(height)
{
	generate_lighting_data();
	create_persistent_buffers(device);
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

	if (width_ != last_width_ || height_ != last_height_)
	{
		create_persistent_buffers(command_buffer.get_device());
		last_width_  = width_;
		last_height_ = height_;
	}

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

	//bin_buffer_->update(bins_);
	command_buffer.bind_buffer(*bin_buffer_, 0, bin_buffer_->get_size(), 0, 7, 0);

	//tile_buffer_->update(tiles_);
	command_buffer.bind_buffer(*tile_buffer_, 0, tile_buffer_->get_size(), 0, 8, 0);

	//light_buffer_->update(light_indices_);
	command_buffer.bind_buffer(*light_buffer_, 0, light_buffer_->get_size(), 0, 9, 0);

	command_buffer.draw(3, 1, 0, 0);
}

void ClusteredLightingPass::create_persistent_buffers(backend::Device &device)
{
	assert(!bins_.empty());
	{
		backend::BufferBuilder buffer_builder{bins_.size() * 4};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

		bin_buffer_ = buffer_builder.build_unique(device);
		bin_buffer_->set_debug_name("ClusteredLightingPass::bins");
		bin_buffer_->update(bins_);
	}
	{
		backend::BufferBuilder buffer_builder{tiles_.size() * 4};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		tile_buffer_ = buffer_builder.build_unique(device);
		tile_buffer_->set_debug_name("ClusteredLightingPass::tiles");
		tile_buffer_->update(tiles_);
	}

	{
		backend::BufferBuilder buffer_builder{light_indices_.size() * 4};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		light_buffer_ = buffer_builder.build_unique(device);
		light_buffer_->set_debug_name("ClusteredLightingPass::light_indices");
		light_buffer_->update(light_indices_);
	}
}

void ClusteredLightingPass::collect_and_sort_lights()
{
	sorted_lights_.clear();
	auto camera_view_proj = camera_.get_pre_rotation() *
	                        camera_.get_view();

	for (uint32_t i = 0; i < lights_.size(); ++i)
	{
		sg::Light *light = lights_[i];
		if (light->get_light_type() != sg::kPoint)
		{
			continue;
		}

		glm::vec4 light_pos         = glm::vec4(light->get_node()->get_transform().get_translation(), 1.0f);
		glm::vec4 projected_pos     = camera_view_proj * light_pos;
		float     range             = light->get_properties().range;
		glm::vec4 projected_pos_min = projected_pos - glm::vec4(0.0f, 0.0f, range, 0.0f);
		glm::vec4 projected_pos_max = projected_pos + glm::vec4(0.0f, 0.0f, range, 0.0f);

		SortedLight sorted_light;
		sorted_light.light_index = i;
		sorted_light.projected_z = (projected_pos.z - camera_.get_near_plane()) /
		                           (camera_.get_far_plane() - camera_.get_near_plane());
		sorted_light.projected_min = (projected_pos_min.z - camera_.get_near_plane()) /
		                             (camera_.get_far_plane() - camera_.get_near_plane());
		sorted_light.projected_max = (projected_pos_max.z - camera_.get_near_plane()) /
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

	for (size_t i = 0; i < sorted_lights_.size(); ++i)
	{
		const auto &light = sorted_lights_[i];
		// calculate the bin range affected by light
		int start_bin = std::max(0, static_cast<int>(light.projected_min / bin_width));
		int end_bin   = std::min(static_cast<int>(num_bins_ - 1),
		                         static_cast<int>(light.projected_max / bin_width));
		// update min/max light index for each affected bin
		for (int bin = start_bin; bin <= end_bin; ++bin)
		{
			uint32_t current_min = (bins_[bin] & 0xFFFF);
			uint32_t current_max = (bins_[bin] >> 16) & 0xFFFF;
			if (current_min == 0xFFFF || i < current_min)
			{
				current_min = i;
			}
			current_max = std::max<size_t>(i, current_max);
			bins_[bin]  = (current_max << 16) | current_min;
		}
	}
}

void ClusteredLightingPass::generate_tiles()
{
	num_tiles_x_ = (width_ + tile_size_ - 1) / tile_size_;
	num_tiles_y_ = (height_ + tile_size_ - 1) / tile_size_;
	// 每个tile需要num_words_个uint32来存储所有light的bitfield
	tiles_.resize(num_tiles_x_ * num_tiles_y_ * num_words_, 0);

	auto camera_view_proj = camera_.get_pre_rotation() *
	                        vulkan_style_projection(camera_.get_projection()) *
	                        camera_.get_view();

	for (size_t sorted_idx = 0; sorted_idx < sorted_lights_.size(); ++sorted_idx)
	{
		const auto &light         = lights_[sorted_lights_[sorted_idx].light_index];
		glm::vec4   light_pos     = glm::vec4(light->get_node()->get_transform().get_translation(), 1.0f);
		glm::vec4   projected_pos = camera_view_proj * light_pos;
		projected_pos /= projected_pos.w;

		float range         = light->get_properties().range;

		if (projected_pos.z < -range)
		{
			continue;
		}

		float screen_radius = range / projected_pos.z;

		// 计算screen space bounds
		int min_x = std::max(0, static_cast<int>((projected_pos.x - screen_radius) * width_ * 0.5f + width_ * 0.5f));
		int max_x = std::min(static_cast<int>(width_ - 1),
		                     static_cast<int>((projected_pos.x + screen_radius) * width_ * 0.5f + width_ * 0.5f));
		int min_y = std::max(0, static_cast<int>((projected_pos.y - screen_radius) * height_ * 0.5f + height_ * 0.5f));
		int max_y = std::min(static_cast<int>(height_ - 1),
		                     static_cast<int>((projected_pos.y + screen_radius) * height_ * 0.5f + height_ * 0.5f));

		// 转换到tile坐标
		int start_tile_x = std::max(0u, min_x / tile_size_);
		int end_tile_x   = std::min(num_tiles_x_ - 1, max_x / tile_size_);
		int start_tile_y = std::max(0u, min_y / tile_size_);
		int end_tile_y   = std::min(num_tiles_y_ - 1, max_y / tile_size_);

		// 计算这个light在bitfield中的位置
		uint32_t word_idx = sorted_idx / 32;
		uint32_t bit_idx  = sorted_idx % 32;

		// 更新所有受这个light影响的tiles
		for (int tile_y = start_tile_y; tile_y <= end_tile_y; ++tile_y)
		{
			for (int tile_x = start_tile_x; tile_x <= end_tile_x; ++tile_x)
			{
				uint32_t tile_idx = tile_y * num_tiles_x_ + tile_x;
				// tile_idx决定基础偏移，word_idx决定在该tile的哪个uint32中设置bit
				uint32_t offset = tile_idx * num_words_ + word_idx;
				tiles_[offset] |= (1u << bit_idx);
			}
		}
	}
}
}        // namespace xihe::rendering
