#include "render_frame.h"

#include "backend/resources_management/resource_caching.h"

constexpr uint32_t kBufferPoolBlockSize = 256;

namespace xihe::rendering
{
RenderFrame::RenderFrame(backend::Device &device, size_t thread_count) :
    device_{device},
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
			buffer_pool_it->second.emplace_back(backend::BufferPool{device, kBufferPoolBlockSize * 1024 * usage_it.second, usage_it.first}, nullptr);
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

RenderTarget &RenderFrame::get_render_target(const std::string &rdg_name)
{
	if (!render_targets_.contains(rdg_name))
	{
		throw std::runtime_error{"Render target not found for RDG name " + rdg_name};
	}
	return *render_targets_[rdg_name];
}

RenderTarget const &RenderFrame::get_render_target(const std::string &rdg_name) const
{
	if (!render_targets_.contains(rdg_name))
	{
		throw std::runtime_error{"Render target not found for RDG name " + rdg_name};
	}
	return *render_targets_.at(rdg_name);
}

vk::DescriptorSet RenderFrame::request_descriptor_set(const backend::DescriptorSetLayout &descriptor_set_layout, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos, bool update_after_bind, size_t thread_index)
{
	assert(thread_index < thread_count_ && "Thread index is out of bounds");

	assert(thread_index < descriptor_pools_.size());
	auto &descriptor_pool = request_resource(device_, nullptr, *descriptor_pools_[thread_index], descriptor_set_layout);
	if (descriptor_management_strategy_ == DescriptorManagementStrategy::kStoreInCache)
	{
		// The bindings we want to update before binding, if empty we update all bindings
		std::vector<uint32_t> bindings_to_update;
		// If update after bind is enabled, we store the binding index of each binding that need to be updated before being bound
		if (update_after_bind)
		{
			bindings_to_update = collect_bindings_to_update(descriptor_set_layout, buffer_infos, image_infos);
		}

		// Request a descriptor set from the render frame, and write the buffer infos and image infos of all the specified bindings
		assert(thread_index < descriptor_sets_.size());
		auto &descriptor_set =
		    request_resource(device_, nullptr, *descriptor_sets_[thread_index], descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
		descriptor_set.update(bindings_to_update);
		return descriptor_set.get_handle();
	}
	else
	{
		// Request a descriptor pool, allocate a descriptor set, write buffer and image data to it
		backend::DescriptorSet descriptor_set{device_, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos};
		descriptor_set.apply_writes();
		return descriptor_set.get_handle();
	}
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

void RenderFrame::update_render_target(std::string rdg_name, std::unique_ptr<RenderTarget> &&render_target)
{
	render_targets_.erase(rdg_name);
	render_targets_.emplace(std::move(rdg_name), std::move(render_target));
}

void RenderFrame::release_owned_semaphore(vk::Semaphore semaphore)
{
	semaphore_pool_.release_owned_semaphore(semaphore);
}

backend::BufferAllocation RenderFrame::allocate_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index)
{
	assert(thread_index < thread_count_ && "Thread index is out of bounds");

	auto buffer_pool_it = buffer_pools_.find(usage);
	if (buffer_pool_it == buffer_pools_.end())
	{
		throw std::runtime_error{"Buffer pool not found for usage " + vk::to_string(usage)};
	}

	assert(thread_index < buffer_pool_it->second.size());
	auto &buffer_pool  = buffer_pool_it->second[thread_index].first;
	auto &buffer_block = buffer_pool_it->second[thread_index].second;

	bool want_minimal_block = buffer_allocation_strategy_ == BufferAllocationStrategy::kOneAllocationPerBuffer;

	if (want_minimal_block || !buffer_block || !buffer_block->can_allocate(size))
	{
		buffer_block = &buffer_pool.request_buffer_block(size, want_minimal_block);
	}

	return buffer_block->allocate(size);
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

std::vector<uint32_t> RenderFrame::collect_bindings_to_update(const backend::DescriptorSetLayout &descriptor_set_layout, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos)
{
	std::set<uint32_t> bindings_to_update;

	auto aggregate_binding_to_update = [&bindings_to_update, &descriptor_set_layout](const auto &infos_map) {
		for (const auto &[binding_index, ignored] : infos_map)
		{
			if (!(descriptor_set_layout.get_layout_binding_flag(binding_index) & vk::DescriptorBindingFlagBits::eUpdateAfterBind))
			{
				bindings_to_update.insert(binding_index);
			}
		}
	};
	aggregate_binding_to_update(buffer_infos);
	aggregate_binding_to_update(image_infos);

	return {bindings_to_update.begin(), bindings_to_update.end()};
}
}        // namespace xihe::rendering
