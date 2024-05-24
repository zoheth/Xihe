#include "vk_common.h"

void xihe::common::image_layout_transition(vk::CommandBuffer command_buffer, vk::Image image, common::ImageMemoryBarrier const &image_memory_barrier, vk::ImageSubresourceRange const &subresource_range)
{
	vk::ImageMemoryBarrier vk_image_memory_barrier(image_memory_barrier.src_access_mask,
	                                               image_memory_barrier.dst_access_mask,
	                                               image_memory_barrier.old_layout,
	                                               image_memory_barrier.new_layout,
	                                               image_memory_barrier.old_queue_family,
	                                               image_memory_barrier.new_queue_family,
	                                               image,
	                                               subresource_range);

	command_buffer.pipelineBarrier(image_memory_barrier.src_stage_mask, image_memory_barrier.dst_stage_mask, {}, {}, {}, vk_image_memory_barrier);
}
