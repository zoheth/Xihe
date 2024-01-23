#include "descriptor_pool.h"

namespace xihe::backend
{
DescriptorPool::DescriptorPool(Device &device, const DescriptorSetLayout &descriptor_set_layout, uint32_t pool_size) :
    device_{device},
    descriptor_set_layout_{&descriptor_set_layout}
{
    
}
}        // namespace xihe::backend
