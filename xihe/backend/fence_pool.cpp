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

	for (vk::Fence fence : fences_)
	{
		device_.get_handle().destroyFence(fence);
	}
	fences_.clear();
}

vk::Fence FencePool::request_fence()
{
	if (active_fence_count_ < fences_.size())
	{
		return fences_[active_fence_count_++];
	}

	vk::Fence fence = device_.get_handle().createFence({});
	fences_.emplace_back(fence);
	active_fence_count_++;

	return fence;
}

vk::Result FencePool::wait(uint32_t timeout) const
{
	if (active_fence_count_ < 1 || fences_.empty())
	{
		return vk::Result::eSuccess;
	}
	return device_.get_handle().waitForFences(active_fence_count_, fences_.data(), VK_TRUE, timeout);
}

vk::Result FencePool::reset()
{
	if (active_fence_count_ < 1 || fences_.empty())
	{
		return vk::Result::eSuccess;
	}
	vk::Result result = device_.get_handle().resetFences(active_fence_count_, fences_.data());

	if (result == vk::Result::eSuccess)
	{
		active_fence_count_ = 0;
	}

	return result;
}
}        // namespace xihe::backend
