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
}        // namespace xihe::rendering
