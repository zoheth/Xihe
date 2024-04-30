#include "descriptor_pool.h"

#include "device.h"

namespace xihe::backend
{
DescriptorPool::DescriptorPool(Device &device, const DescriptorSetLayout &descriptor_set_layout, uint32_t pool_size) :
    device_{device},
    descriptor_set_layout_{&descriptor_set_layout}
{
    
}

const DescriptorSetLayout & DescriptorPool::get_descriptor_set_layout() const
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
	// todo
	return vk::DescriptorSet{};
}
}        // namespace xihe::backend
