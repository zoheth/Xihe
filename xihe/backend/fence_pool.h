#pragma once

#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;

class FencePool
{
public:
	FencePool(Device &device);
	~FencePool();

	FencePool(const FencePool &)            = delete;
	FencePool &operator=(const FencePool &) = delete;
	FencePool(FencePool &&)                 = delete;
	FencePool &operator=(FencePool &&)      = delete;

	vk::Fence request_fence();

	vk::Result wait(uint32_t timeout = std::numeric_limits<uint32_t>::max()) const;

	vk::Result reset();

  private:
	Device &device_;

	std::vector<vk::Fence> fences_;

	uint32_t active_fence_count_{0};
};
}