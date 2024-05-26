#pragma once

#include <vulkan/vulkan.hpp>

namespace xihe
{
namespace backend
{
class CommandBuffer;
class Device;

class Queue
{
public:
	Queue(Device &device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present, uint32_t index);

	Queue(const Queue &) = default;
	Queue(Queue &&other) noexcept;

	Queue &operator=(const Queue &) = delete;
	Queue &operator=(Queue &&) = delete;

	const Device &get_device() const;

	vk::Queue get_handle() const;

	uint32_t get_family_index() const;
	uint32_t get_index() const;

	const vk::QueueFamilyProperties &get_properties() const;

	vk::Bool32 support_present() const;

	vk::Result submit(const std::vector<vk::SubmitInfo> &submit_infos, vk::Fence fence) const;

	void submit(const CommandBuffer &command_buffer, vk::Fence fence) const;

	vk::Result present(vk::PresentInfoKHR &present_info) const;

private:
	Device &device_;
	vk::Queue handle_;

	uint32_t family_index_{0};
	uint32_t index_{0};

	vk::Bool32 can_present_{VK_FALSE};

	vk::QueueFamilyProperties properties_{};

};
}
}        // namespace xihe