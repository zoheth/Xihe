#include "descriptor_set_layout.h"

namespace xihe::backend
{
DescriptorSetLayout::DescriptorSetLayout(Device &device, const uint32_t set_index, const std::vector<ShaderModule *> &shader_modules, const std::vector<ShaderResource> &resource_set) :
    device_{device},
    set_index_{set_index},
    shader_modules_{shader_modules}
{
	
}
}        // namespace xihe::backend
