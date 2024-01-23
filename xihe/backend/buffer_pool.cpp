#include "buffer_pool.h"

namespace xihe::backend
{
BufferPool::BufferPool(Device &device, vk::DeviceSize block_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage) :
    device_{device},
    block_size_{block_size},
    usage_{usage},
    memory_usage_{memory_usage}
{
}
}        // namespace xihe::backend
