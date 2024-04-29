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
};

class BufferBlock
{
  public:
	BufferBlock(Device &device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage);

	void reset();

  private:
	backend::Buffer buffer_;
	vk::DeviceSize  alignment{0};
	vk::DeviceSize  offset_{0};
};

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
