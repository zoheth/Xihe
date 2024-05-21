#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;
class ShaderModule;
struct ShaderResource;

class DescriptorSetLayout
{
  public:
	DescriptorSetLayout(Device                            &device,
	                    const uint32_t                     set_index,
	                    const std::vector<ShaderModule *> &shader_modules,
	                    const std::vector<ShaderResource> &resource_set);

	DescriptorSetLayout(const DescriptorSetLayout &) = delete;

	DescriptorSetLayout(DescriptorSetLayout &&other);

	~DescriptorSetLayout();

	vk::DescriptorSetLayout get_handle() const;

	uint32_t get_index() const;

	const std::vector<vk::DescriptorSetLayoutBinding> &get_bindings() const;

	std::unique_ptr<vk::DescriptorSetLayoutBinding> get_layout_binding(const uint32_t binding_index) const;

	std::unique_ptr<vk::DescriptorSetLayoutBinding> get_layout_binding(const std::string &name) const;

	const std::vector<vk::DescriptorBindingFlagsEXT> &get_binding_flags() const;

	vk::DescriptorBindingFlagsEXT get_layout_binding_flag(const uint32_t binding_index) const;

	const std::vector<ShaderModule *> &get_shader_modules() const;

  private:
	Device        &device_;
	const uint32_t set_index_;

	vk::DescriptorSetLayout handle_{VK_NULL_HANDLE};

	std::vector<vk::DescriptorSetLayoutBinding> bindings_;
	std::vector<vk::DescriptorBindingFlagsEXT>  binding_flags_;

	std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings_lookup_;
	std::unordered_map<uint32_t, vk::DescriptorBindingFlagsEXT>  binding_flags_lookup_;
	std::unordered_map<std::string, uint32_t>                    resources_lookup_;

	std::vector<ShaderModule *> shader_modules_;
};
}        // namespace xihe::backend
