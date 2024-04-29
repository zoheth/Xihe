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

		SemaphorePool(const SemaphorePool &) = delete;

	    SemaphorePool(SemaphorePool &&other) = delete;

	    ~SemaphorePool();

	    SemaphorePool &operator=(const SemaphorePool &) = delete;

	    SemaphorePool &operator=(SemaphorePool &&) = delete;

	    vk::Semaphore request_semaphore();
	    vk::Semaphore request_semaphore_with_ownership();
	    void        release_owned_semaphore(vk::Semaphore semaphore);

	    void reset();

	    uint32_t get_active_semaphore_count() const;

	private:
		Device &device_;

		std::vector<vk::Semaphore> semaphores_;
		std::vector<vk::Semaphore> released_semaphores_;

		uint32_t active_semaphore_count_{0};
	};
}