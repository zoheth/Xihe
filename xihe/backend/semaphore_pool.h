#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;
	class SemaphorePool
	{
	public:
		explicit SemaphorePool(Device &device);

	private:
		Device &device_;

		std::vector<vk::Semaphore> semaphores_;
		std::vector<vk::Semaphore> released_semaphores_;

		uint32_t active_semaphore_index_{0};
	};
}