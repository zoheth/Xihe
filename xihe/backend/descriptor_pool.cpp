#include "descriptor_pool.h"

#include "device.h"

namespace xihe::backend
{
DescriptorPool::DescriptorPool(Device &device, const DescriptorSetLayout &descriptor_set_layout, uint32_t pool_size) :
    device_{device},
    descriptor_set_layout_{&descriptor_set_layout}
{
	const auto &bindings = descriptor_set_layout.get_bindings();

	std::map<vk::DescriptorType, std::uint32_t> descriptor_type_counts;

	for (auto &binding : bindings)
	{
		descriptor_type_counts[binding.descriptorType] += binding.descriptorCount;
	}

	pool_sizes_.resize(descriptor_type_counts.size());

	auto it = pool_sizes_.begin();

	for (const auto &descriptor_type_count : descriptor_type_counts)
	{
		it->type            = descriptor_type_count.first;
		it->descriptorCount = descriptor_type_count.second * pool_size;
		++it;
	}

	pool_max_sets_ = pool_size;
}

DescriptorPool::~DescriptorPool()
{
	for (auto pool : pools_)
	{
		device_.get_handle().destroyDescriptorPool(pool);
	}
}

const DescriptorSetLayout &DescriptorPool::get_descriptor_set_layout() const
{
	assert(descriptor_set_layout_ && "Descriptor set layout is invalid");
	return *descriptor_set_layout_;
}

void DescriptorPool::set_descriptor_set_layout(const DescriptorSetLayout &set_layout)
{
	descriptor_set_layout_ = &set_layout;
}

void DescriptorPool::reset()
{
	for (auto pool : pools_)
	{
		device_.get_handle().destroyDescriptorPool(pool);
	}

	std::ranges::fill(pool_sets_count_, 0);
	set_pool_mapping_.clear();

	pool_index_ = 0;
}

vk::DescriptorSet DescriptorPool::allocate()
{
	pool_index_ = find_available_pool(pool_index_);

	++pool_sets_count_[pool_index_];

	vk::DescriptorSetLayout set_layout = get_descriptor_set_layout().get_handle();

	vk::DescriptorSetAllocateInfo allocate_info{};
	allocate_info.descriptorPool     = pools_[pool_index_];
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts        = &set_layout;

	vk::DescriptorSet handle{};

	auto result = device_.get_handle().allocateDescriptorSets(&allocate_info, &handle);

	if (result != vk::Result::eSuccess)
	{
		--pool_sets_count_[pool_index_];
		return VK_NULL_HANDLE;
	}

	set_pool_mapping_.emplace(handle, pool_index_);

	return handle;
}

uint32_t DescriptorPool::find_available_pool(uint32_t pool_index)
{
	if (pools_.size() <= pool_index)
	{
		vk::DescriptorPoolCreateInfo pool_create_info{};
		pool_create_info.maxSets       = pool_max_sets_;
		pool_create_info.poolSizeCount = to_u32(pool_sizes_.size());
		pool_create_info.pPoolSizes    = pool_sizes_.data();

		// We do not set FREE_DESCRIPTOR_SET_BIT as we do not need to free individual descriptor sets
		pool_create_info.flags = {};

		auto &binding_flags = descriptor_set_layout_->get_binding_flags();
		for (auto binding_flag : binding_flags)
		{
			if (binding_flag & vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind)
			{
				pool_create_info.flags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
				break;
			}
		}

		vk::DescriptorPool handle{};
		auto               result = device_.get_handle().createDescriptorPool(&pool_create_info, nullptr, &handle);

		if (result != vk::Result::eSuccess)
		{
			return 0;
		}

		pools_.push_back(handle);

		pool_sets_count_.push_back(0);

		return pool_index;
	}
	else if (pool_sets_count_[pool_index] < pool_max_sets_)
	{
		return pool_index;
	}
	return find_available_pool(pool_index + 1);
}


}        // namespace xihe::backend
