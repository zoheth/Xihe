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
		auto [buffer_pool_it, inserted] = buffer_pools.emplace(usage_it.first, std::vector<std::pair<backend::BufferPool, backend::BufferBlock *>>{});
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

backend::CommandBuffer & RenderFrame::request_command_buffer(const backend::Queue &queue, backend::CommandBuffer::ResetMode reset_mode, vk::CommandBufferLevel level, size_t thread_index)
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

std::vector<std::unique_ptr<backend::CommandPool>> & RenderFrame::get_command_pools(const backend::Queue &queue, backend::CommandBuffer::ResetMode reset_mode)
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
	}
	else
	{
		return command_pool_it->second;
	}

	bool inserted = false;
	std::tie(command_pool_it, inserted) = command_pools_.emplace(queue.get_family_index(), std::vector<std::unique_ptr<backend::CommandPool>>{});
	if (!inserted)
	{
		throw std::runtime_error{"Command pool already exists for queue family " + std::to_string(queue.get_family_index())};
	}

	for (size_t i = 0; i < thread_count_; ++i)
	{
		command_pool_it->second.push_back(std::make_unique<backend::CommandPool>(device_, queue.get_family_index(), this, i, reset_mode));
	}

	return command_pool_it->second;
}

}        // namespace xihe::rendering
