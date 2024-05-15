#include "render_frame.h"

constexpr uint32_t kBufferPoolBlockSize = 256;

namespace xihe::rendering
{
RenderFrame::RenderFrame(backend::Device &device, std::unique_ptr<RenderTarget> &&render_target, size_t thread_count) :
    device_{device},
    swapchain_render_target_{std::move(render_target)},
    fence_pool_{device},
    semaphore_pool_{device},
    thread_count_{thread_count}
{
	for (auto &usage_it : supported_usage_map_)
	{
		auto [buffer_pool_it, inserted] = buffer_pools_.emplace(usage_it.first, std::vector<std::pair<backend::BufferPool, backend::BufferBlock *>>{});
		if (!inserted)
		{
			throw std::runtime_error{"Buffer pool already exists for usage " + vk::to_string(usage_it.first)};
		}

		for (size_t i = 0; i < thread_count; ++i)
		{
			buffer_pool_it->second.push_back(std::make_pair(backend::BufferPool{device, kBufferPoolBlockSize * 1024 * usage_it.second, usage_it.first}, nullptr));
		}
	}

	for (size_t i = 0; i < thread_count; ++i)
	{
		descriptor_pools_.push_back(std::make_unique<std::unordered_map<std::size_t, backend::DescriptorPool>>());
		descriptor_sets_.push_back(std::make_unique<std::unordered_map<std::size_t, backend::DescriptorSet>>());
	}
}

backend::CommandBuffer &RenderFrame::request_command_buffer(const backend::Queue &queue, backend::CommandBuffer::ResetMode reset_mode, vk::CommandBufferLevel level, size_t thread_index)
{
	assert(thread_index < thread_count_ && "Thread index is out of bounds");

	auto &command_pools = get_command_pools(queue, reset_mode);

	auto command_pool_it = std::ranges::find_if(command_pools,
	                                            [&thread_index](std::unique_ptr<backend::CommandPool> &cmd_pool) {
		                                            return cmd_pool->get_thread_index() == thread_index;
	                                            });

	assert(command_pool_it != command_pools.end());

	return (*command_pool_it)->request_command_buffer(level);
}

RenderTarget &RenderFrame::get_render_target()
{
	return *swapchain_render_target_;
}

RenderTarget const &RenderFrame::get_render_target() const
{
	return *swapchain_render_target_;
}

vk::Fence RenderFrame::request_fence()
{
	return fence_pool_.request_fence();
}

vk::Semaphore RenderFrame::request_semaphore()
{
	return semaphore_pool_.request_semaphore();
}

vk::Semaphore RenderFrame::request_semaphore_with_ownership()
{
	return semaphore_pool_.request_semaphore_with_ownership();
}

void RenderFrame::reset()
{
	vk::Result result = fence_pool_.wait();
	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error{"Failed to wait for fences"};
	}

	fence_pool_.reset();

	for (auto &command_pools_per_queue : command_pools_)
	{
		for (auto &command_pool : command_pools_per_queue.second)
		{
			command_pool->reset_pool();
		}
	}

	for (auto &buffer_pools_per_usage : buffer_pools_)
	{
		for (auto &buffer_pool : buffer_pools_per_usage.second)
		{
			buffer_pool.first.reset();
			buffer_pool.second = nullptr;
		}
	}

	semaphore_pool_.reset();

	if (descriptor_management_strategy_ == DescriptorManagementStrategy::kCreateDirectly)
	{
		clear_descriptors();
	}
}

void RenderFrame::clear_descriptors()
{
	for (auto &desc_sets_per_thread : descriptor_sets_)
	{
		desc_sets_per_thread->clear();
	}

	for (auto &desc_pools_per_thread : descriptor_pools_)
	{
		for (auto &desc_pool : *desc_pools_per_thread)
		{
			desc_pool.second.reset();
		}
	}
}

void RenderFrame::update_render_target(std::unique_ptr<RenderTarget> &&render_target)
{
	swapchain_render_target_ = std::move(render_target);
}

void RenderFrame::release_owned_semaphore(vk::Semaphore semaphore)
{
	semaphore_pool_.release_owned_semaphore(semaphore);
}

std::vector<std::unique_ptr<backend::CommandPool>> &RenderFrame::get_command_pools(const backend::Queue &queue, backend::CommandBuffer::ResetMode reset_mode)
{
	auto command_pool_it = command_pools_.find(queue.get_family_index());

	if (command_pool_it != command_pools_.end())
	{
		assert(!command_pool_it->second.empty());
		if (command_pool_it->second[0]->get_reset_mode() != reset_mode)
		{
			device_.get_handle().waitIdle();

			// Delete pools
			command_pools_.erase(command_pool_it);
		}

		else
		{
			return command_pool_it->second;
		}
	}

	std::vector<std::unique_ptr<backend::CommandPool>> queue_command_pools;
	for (size_t i = 0; i < thread_count_; i++)
	{
		queue_command_pools.push_back(std::make_unique<backend::CommandPool>(device_, queue.get_family_index(), this, i, reset_mode));
	}

	auto res_ins_it = command_pools_.emplace(queue.get_family_index(), std::move(queue_command_pools));

	if (!res_ins_it.second)
	{
		throw std::runtime_error("Failed to insert command pool");
	}

	command_pool_it = res_ins_it.first;

	return command_pool_it->second;
}

}        // namespace xihe::rendering
