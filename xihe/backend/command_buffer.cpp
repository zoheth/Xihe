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


}        // namespace xihe::backend
