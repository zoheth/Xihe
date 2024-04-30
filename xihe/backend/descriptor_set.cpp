#include "descriptor_set.h"

#include "descriptor_pool.h"

namespace xihe::backend
{
DescriptorSet::DescriptorSet(Device &device, const DescriptorSetLayout &descriptor_set_layout, DescriptorPool &descriptor_pool, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos) :
    device_{device},
    descriptor_set_layout_{descriptor_set_layout},
    descriptor_pool_{descriptor_pool},
    buffer_infos_{buffer_infos},
    image_infos_{image_infos},
    handle_{descriptor_pool.allocate()}
{}

const DescriptorSetLayout &DescriptorSet::get_layout() const
{
	return descriptor_set_layout_;
}

vk::DescriptorSet DescriptorSet::get_handle() const
{
	return handle_;
}
}        // namespace xihe::backend
