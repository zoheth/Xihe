#include "descriptor_set.h"

namespace xihe::backend
{
const DescriptorSetLayout &DescriptorSet::get_layout() const
{
	return descriptor_set_layout_;
}

vk::DescriptorSet DescriptorSet::get_handle() const
{
	return handle_;
}
}        // namespace xihe::backend
