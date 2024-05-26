#include "queue.h"

#include "backend/device.h"

namespace xihe
{
namespace backend
{
Queue::Queue(Device &device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present, uint32_t index) :
    device_{device},
    family_index_{family_index},
    index_{index},
    can_present_{can_present},
    properties_{properties}
{
	handle_ = device_.get_handle().getQueue(family_index_, index_);
}

Queue::Queue(Queue &&other) noexcept :
    device_(other.device_),
    handle_(std::exchange(other.handle_, {})),
    family_index_(std::exchange(other.family_index_, {})),
    index_(std::exchange(other.index_, 0)),
    can_present_(std::exchange(other.can_present_, false)),
    properties_(std::exchange(other.properties_, {}))
{
}

const Device &Queue::get_device() const
{
	return device_;
}

vk::Queue Queue::get_handle() const
{
	return handle_;
}

uint32_t Queue::get_family_index() const
{
	return family_index_;
}

uint32_t Queue::get_index() const
{
	return index_;
}

const vk::QueueFamilyProperties &Queue::get_properties() const
{
	return properties_;
}

vk::Bool32 Queue::support_present() const
{
	return can_present_;
}

vk::Result Queue::submit(const std::vector<vk::SubmitInfo> &submit_infos, vk::Fence fence) const
{
	handle_.submit(submit_infos, fence);
    return vk::Result::eSuccess;
}

void Queue::submit(const CommandBuffer &command_buffer, vk::Fence fence) const
{
	vk::CommandBuffer vk_command_buffer = command_buffer.get_handle();
	vk::SubmitInfo submit_info{
	    {},
	    {},
        vk_command_buffer};

	handle_.submit(submit_info, fence);
}

vk::Result Queue::present(vk::PresentInfoKHR &present_info) const
{
	if (!can_present_)
	{
		return vk::Result::eErrorIncompatibleDisplayKHR;
	}

	return handle_.presentKHR(present_info);
}
}        // namespace backend
}        // namespace xihe
