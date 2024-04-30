#include "buffer_pool.h"

#include "device.h"

namespace xihe::backend
{
BufferBlock::BufferBlock(Device &device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
	BufferBuilder builder{size};
	builder.with_usage(usage);
	builder.with_vma_usage(memory_usage);
	buffer_ =  builder.build_unique(device);

	if (usage == vk::BufferUsageFlagBits::eUniformBuffer)
	{
		alignment = device.get_gpu().get_properties().limits.minUniformBufferOffsetAlignment;
	}
	else if (usage == vk::BufferUsageFlagBits::eStorageBuffer)
	{
		alignment = device.get_gpu().get_properties().limits.minStorageBufferOffsetAlignment;
	}
	else if (usage = vk::BufferUsageFlagBits::eUniformTexelBuffer)
	{
		alignment = device.get_gpu().get_properties().limits.minTexelBufferOffsetAlignment;
	}
	else if (usage == vk::BufferUsageFlagBits::eIndexBuffer ||
	         usage == vk::BufferUsageFlagBits::eVertexBuffer ||
	         usage == vk::BufferUsageFlagBits::eIndirectBuffer)
	{
		// Used to calculate the offset, required when allocating memory (its value should be power of 2)
		alignment = 16;
	}
	else
	{
		throw std::runtime_error{"Unsupported buffer usage"};
	}
}

void BufferBlock::reset()
{
	offset_ = 0;
}

BufferPool::BufferPool(Device &device, vk::DeviceSize block_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage) :
    device_{device},
    block_size_{block_size},
    usage_{usage},
    memory_usage_{memory_usage}
{
}

//BufferBlock &BufferPool::request_buffer_block(vk::DeviceSize minimum_size, bool minimal)
//{}

void BufferPool::reset()
{
	for (auto &buffer_block : buffer_blocks_)
	{
		buffer_block->reset();
	}
}
}        // namespace xihe::backend
