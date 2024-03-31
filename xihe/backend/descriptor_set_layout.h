#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;
class ShaderModule;
class ShaderResource;

class DescriptorSetLayout
{
  public:
	DescriptorSetLayout(Device                            &device,
	                    const uint32_t                     set_index,
	                    const std::vector<ShaderModule *> &shader_modules,
	                    const std::vector<ShaderResource> &resource_set);

	vk::DescriptorSetLayout get_handle() const;

  private:
	Device        &device_;
	const uint32_t set_index_;

	vk::DescriptorSetLayout handle_{VK_NULL_HANDLE};

	std::vector<vk::DescriptorSetLayoutBinding> bindings_;
	std::vector<vk::DescriptorBindingFlagsEXT>  binding_flags_;

	std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> binding_lookup_;
	std::unordered_map<uint32_t, vk::DescriptorBindingFlagsEXT>  binding_flags_lookup_;
	std::unordered_map<std::string, uint32_t>                    resources_lookup_;

	std::vector<ShaderModule *> shader_modules_;
};
}        // namespace xihe::backend
