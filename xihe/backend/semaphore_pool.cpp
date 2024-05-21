#include "semaphore_pool.h"

#include "backend/device.h"

namespace xihe::backend
{
SemaphorePool::SemaphorePool(Device &device) :
    device_{device}
{
}

SemaphorePool::~SemaphorePool()
{
	reset();

	for (vk::Semaphore semaphore : semaphores_)
	{
		device_.get_handle().destroySemaphore(semaphore);
	}

	semaphores_.clear();
}

vk::Semaphore SemaphorePool::request_semaphore()
{
	if (active_semaphore_count_ < semaphores_.size())
	{
		return semaphores_[active_semaphore_count_++];
	}

	vk::Semaphore           semaphore;
	vk::SemaphoreCreateInfo create_info{};

	vk::Result result = device_.get_handle().createSemaphore(&create_info, nullptr, &semaphore);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error{"Failed to create semaphore"};
	}

	semaphores_.push_back(semaphore);

	active_semaphore_count_++;

	return semaphore;
}

vk::Semaphore SemaphorePool::request_semaphore_with_ownership()
{
	if (active_semaphore_count_ < semaphores_.size())
	{
		auto semaphore = semaphores_.back();
		semaphores_.pop_back();
		return semaphore;
	}

	vk::Semaphore           semaphore;
	vk::SemaphoreCreateInfo create_info{};

	vk::Result result = device_.get_handle().createSemaphore(&create_info, nullptr, &semaphore);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error{"Failed to create semaphore"};
	}

	return semaphore;
}

void SemaphorePool::release_owned_semaphore(vk::Semaphore semaphore)
{
	// We cannot reuse this semaphore until ::reset().
	released_semaphores_.push_back(semaphore);
}

void SemaphorePool::reset()
{
	active_semaphore_count_ = 0;

	for (vk::Semaphore semaphore : released_semaphores_)
	{
		semaphores_.push_back(semaphore);
	}

	released_semaphores_.clear();
}

uint32_t SemaphorePool::get_active_semaphore_count() const
{
	return active_semaphore_count_;
}
}        // namespace xihe::backend
