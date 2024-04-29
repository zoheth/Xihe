#include "command_buffer.h"

#include "backend/command_pool.h"
#include "backend/device.h"
#include "vulkan/vulkan_format_traits.hpp"

namespace xihe::backend
{
CommandBuffer::CommandBuffer(CommandPool &command_pool, vk::CommandBufferLevel level) :
    VulkanResource(nullptr, &command_pool.get_device()),
    level_{level},
    command_pool_{command_pool},
    max_push_constants_size_{get_device().get_gpu().get_properties().limits.maxPushConstantsSize}

{
	vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(), level, 1);

	set_handle(get_device().get_handle().allocateCommandBuffers(allocate_info).front());
}

CommandBuffer::CommandBuffer(CommandBuffer &&other) noexcept :
    VulkanResource(std::move(other)),
    level_{other.level_},
    command_pool_{other.command_pool_},
    max_push_constants_size_{other.max_push_constants_size_}
{}

CommandBuffer::~CommandBuffer()
{
	if (get_handle())
	{
		get_device().get_handle().freeCommandBuffers(command_pool_.get_handle(), get_handle());
	}
}

vk::Result CommandBuffer::begin(vk::CommandBufferUsageFlags flags, CommandBuffer *primary_cmd_buf)
{
	if (level_ == vk::CommandBufferLevel::eSecondary)
	{
		// todo
		assert(false);
	}
	return begin(flags, nullptr, nullptr, 0);
}

vk::Result CommandBuffer::begin(vk::CommandBufferUsageFlags flags, const backend::RenderPass *render_pass, const backend::Framebuffer *framebuffer, uint32_t subpass_info)
{
	pipeline_state_.reset();
	resource_binding_state_.reset();
	descriptor_set_layout_binding_state_.clear();
	stored_push_constants_.clear();

	vk::CommandBufferBeginInfo       begin_info(flags);
	vk::CommandBufferInheritanceInfo inheritance_info;

	if (level_ == vk::CommandBufferLevel::eSecondary)
	{
		// todo
		assert(false);
	}

	get_handle().begin(begin_info);
	return vk::Result::eSuccess;
}

vk::Result CommandBuffer::end()
{
	get_handle().end();
	return vk::Result::eSuccess;
}

void CommandBuffer::image_memory_barrier(const ImageView &image_view, const ImageMemoryBarrier &memory_barrier)
{
	auto subresource_range = image_view.get_subresource_range();
	auto format            = image_view.get_format();

	// Adjust the aspect mask if the format is a depth format
	if (is_depth_only_format(format))
	{
		subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
	}
	else if (is_depth_stencil_format(format))
	{
		subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	}

	vk::ImageMemoryBarrier image_memory_barrier(memory_barrier.src_access_mask,
	                                            memory_barrier.dst_access_mask,
	                                            memory_barrier.old_layout,
	                                            memory_barrier.new_layout,
	                                            memory_barrier.old_queue_family,
	                                            memory_barrier.new_queue_family,
	                                            image_view.get_image().get_handle(),
	                                            subresource_range);

	vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
	vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

	get_handle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, image_memory_barrier);
}

vk::Result CommandBuffer::reset(ResetMode reset_mode)
{
	assert(reset_mode == command_pool_.get_reset_mode() && "Command buffer reset mode must match the one used by the pool to allocate it");

	if (reset_mode == ResetMode::kResetIndividually)
	{
		get_handle().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	}

	return vk::Result::eSuccess;
}
}        // namespace xihe::backend
