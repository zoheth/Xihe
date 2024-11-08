#include "vk_common.h"

namespace xihe::common
{
void image_layout_transition(vk::CommandBuffer command_buffer, vk::Image image, common::ImageMemoryBarrier const &image_memory_barrier, vk::ImageSubresourceRange const &subresource_range)
{
	vk::ImageMemoryBarrier2 vk_image_memory_barrier(image_memory_barrier.src_stage_mask,
	                                                image_memory_barrier.src_access_mask,
	                                                image_memory_barrier.dst_stage_mask,
	                                                image_memory_barrier.dst_access_mask,
	                                                image_memory_barrier.old_layout,
	                                                image_memory_barrier.new_layout,
	                                                image_memory_barrier.old_queue_family,
	                                                image_memory_barrier.new_queue_family,
	                                                image,
	                                                subresource_range);
	vk::DependencyInfo      dependency_info{};
	dependency_info.setImageMemoryBarriers({vk_image_memory_barrier});

	command_buffer.pipelineBarrier2(dependency_info);
}

void make_filters_valid(vk::PhysicalDevice physical_device, vk::Format format, vk::Filter *filter, vk::SamplerMipmapMode *mipmap_mode)
{
	if (*filter == vk::Filter::eNearest && (mipmap_mode == nullptr || *mipmap_mode == vk::SamplerMipmapMode::eNearest))
	{
		return;
	}

	vk::FormatProperties properties;
	physical_device.getFormatProperties(format, &properties);

	if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
	{
		*filter = vk::Filter::eNearest;
		if (mipmap_mode != nullptr)
		{
						*mipmap_mode = vk::SamplerMipmapMode::eNearest;
		}
	}
}
}
