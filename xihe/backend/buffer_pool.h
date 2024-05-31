#pragma once
#include "buffer.h"
#include "vk_mem_alloc.h"

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace xihe::backend
{

class Device;

/**
 * \brief An allocation of vulkan memory; different buffer allocations,
 *        with different offset and size, may come from the same Vulkan buffer
 */
class BufferAllocation
{
  public:
	BufferAllocation() = default;

	BufferAllocation(backend::Buffer &buffer, vk::DeviceSize size, vk::DeviceSize offset);

	BufferAllocation(const BufferAllocation &) = delete;

	BufferAllocation(BufferAllocation &&) = default;

	BufferAllocation &operator=(const BufferAllocation &) = delete;

	BufferAllocation &operator=(BufferAllocation &&) = default;

	void update(const std::vector<uint8_t> &data, uint32_t offset = 0);

	template <class T>
	void update(const T &value, uint32_t offset = 0)
	{
		update(to_bytes(value), offset);
	}

	bool empty() const;

	vk::DeviceSize get_size() const;

	vk::DeviceSize get_offset() const;

	backend::Buffer &get_buffer() const;

  private:
	backend::Buffer *buffer_{nullptr};

	VkDeviceSize base_offset_{0};

	VkDeviceSize size_{0};
};

class BufferBlock
{
  public:
	BufferBlock(Device &device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage);

	bool can_allocate(vk::DeviceSize size) const;

	BufferAllocation allocate(vk::DeviceSize size);

	vk::DeviceSize get_size() const;

	void reset();

  private:
	vk::DeviceSize aligned_offset() const;

  private:
	BufferPtr      buffer_;
	vk::DeviceSize alignment{0};
	vk::DeviceSize offset_{0};
};

inline vk::DeviceSize BufferBlock::aligned_offset() const
{
	return (offset_ + alignment - 1) & ~(alignment - 1);
}

class BufferPool
{
  public:
	BufferPool(Device &device, vk::DeviceSize block_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);

	BufferBlock &request_buffer_block(vk::DeviceSize minimum_size, bool minimal = false);

	void reset();

  private:
	Device &device_;

	std::vector<std::unique_ptr<BufferBlock>> buffer_blocks_;

	vk::DeviceSize       block_size_{0};
	vk::BufferUsageFlags usage_{};
	VmaMemoryUsage       memory_usage_{};
};
}        // namespace xihe::backend
