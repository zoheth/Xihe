#include "buffer_pool.h"

#include "device.h"

namespace xihe::backend
{

BufferAllocation::BufferAllocation(backend::Buffer &buffer, vk::DeviceSize size, vk::DeviceSize offset) :
    buffer_{&buffer},
    base_offset_{offset},
    size_{size}
{
}

void BufferAllocation::update(const std::vector<uint8_t> &data, uint32_t offset)
{
	assert(buffer_ && "Invalid buffer pointer");

	if (offset + data.size() <= size_)
	{
		buffer_->update(data, to_u32(base_offset_) + offset);
	}
	else
	{
		LOGE("Ignore buffer allocation update");
	}
}

bool BufferAllocation::empty() const
{
	return size_ == 0 || buffer_ == nullptr;
}

VkDeviceSize BufferAllocation::get_size() const
{
	return size_;
}

VkDeviceSize BufferAllocation::get_offset() const
{
	return base_offset_;
}

backend::Buffer &BufferAllocation::get_buffer() const
{
	assert(buffer_ && "Invalid buffer pointer");
	return *buffer_;
}

BufferBlock::BufferBlock(Device &device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
	BufferBuilder builder{size};
	builder.with_usage(usage);
	builder.with_vma_usage(memory_usage);
	buffer_ = builder.build_unique(device);

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

bool BufferBlock::can_allocate(vk::DeviceSize size) const
{
	assert(size > 0 && "Allocation size must be greater than zero");
	return (aligned_offset() + size <= buffer_->get_size());
}

BufferAllocation BufferBlock::allocate(vk::DeviceSize size)
{
	assert(can_allocate(size) && "Cannot allocate requested size");

	auto aligned = aligned_offset();
	offset_      = aligned + size;

	return BufferAllocation{*buffer_, size, aligned};
}

vk::DeviceSize BufferBlock::get_size() const
{
	return buffer_->get_size();
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

BufferBlock &BufferPool::request_buffer_block(vk::DeviceSize minimum_size, bool minimal)
{
	// Find a block in the range of the blocks which can fit the minimum size
	auto it = minimal ? std::ranges::find_if(buffer_blocks_,
	                                         [&minimum_size](const std::unique_ptr<BufferBlock> &buffer_block) { return (buffer_block->get_size() == minimum_size) && buffer_block->can_allocate(minimum_size); }) :
	                    std::ranges::find_if(buffer_blocks_,
	                                         [&minimum_size](const std::unique_ptr<BufferBlock> &buffer_block) { return buffer_block->can_allocate(minimum_size); });

	if (it == buffer_blocks_.end())
	{
		LOGD("Building #{} buffer block ({})", buffer_blocks_.size(), to_string(usage_));

		vk::DeviceSize new_block_size = minimal ? minimum_size : std::max(block_size_, minimum_size);

		// Create a new block and get the iterator on it
		it = buffer_blocks_.emplace(buffer_blocks_.end(), std::make_unique<BufferBlock>(device_, new_block_size, usage_, memory_usage_));
	}

	return *it->get();
}

void BufferPool::reset()
{
	for (auto &buffer_block : buffer_blocks_)
	{
		buffer_block->reset();
	}
}
}        // namespace xihe::backend
