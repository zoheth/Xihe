#include "fence_pool.h"

#include "backend/device.h"

namespace xihe::backend
{
FencePool::FencePool(Device &device) :
    device_(device)
{}

FencePool::~FencePool()
{
	wait();
	reset();
}

vk::Fence FencePool::request_fence()
{
	if (active_fence_count_ < fences_.size())
	{
		return fences_[active_fence_count_++];
	}

	fences_.emplace_back(device_.get_handle().createFence({}));
	return fences_[active_fence_count_++];
}

vk::Result FencePool::wait(uint32_t timeout) const
{
	if (active_fence_count_ < 1 || fences_.empty())
	{
		return vk::Result::eSuccess;
	}
	return device_.get_handle().waitForFences(active_fence_count_, fences_.data(), VK_TRUE, timeout);
}

vk::Result FencePool::reset() const
{
	if (active_fence_count_ < 1 || fences_.empty())
	{
				return vk::Result::eSuccess;
	}
	return device_.get_handle().resetFences(active_fence_count_, fences_.data());
}
}        // namespace xihe::backend
