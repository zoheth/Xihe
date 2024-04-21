#include "descriptor_pool.h"

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
}        // namespace xihe::backend
