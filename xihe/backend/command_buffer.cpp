#include "command_buffer.h"

#include "backend/command_pool.h"
#include "backend/device.h"

namespace xihe::backend
{
CommandBuffer::CommandBuffer(CommandPool &command_pool, vk::CommandBufferLevel level) :
    VulkanResource(nullptr, &command_pool.get_device()),
    level_{level},
    command_pool_{command_pool},
    max_push_constant_size_{get_device().get_gpu().get_properties().limits.maxPushConstantsSize}

{
	vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(), level, 1);

    set_handle(get_device().get_handle().allocateCommandBuffers(allocate_info).front());
}

CommandBuffer::CommandBuffer(CommandBuffer &&other) noexcept :
	VulkanResource(std::move(other)),
	level_{other.level_},
	command_pool_{other.command_pool_},
	max_push_constant_size_{other.max_push_constant_size_}
{}

CommandBuffer::~CommandBuffer()
{
	if (get_handle())
	{
		get_device().get_handle().freeCommandBuffers(command_pool_.get_handle(), get_handle());
	}
}

vk::Result CommandBuffer::begin(vk::CommandBufferUsageFlags flags, CommandBuffer *primary_cmd_buf)
{}
}        // namespace xihe::backend
