#pragma once

#include "shader_module.h"

#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;
class ShaderModule;
class DescriptorSetLayout;

class PipelineLayout
{
  public:
	PipelineLayout(Device &device, const std::vector<ShaderModule *> &shader_modules);

	PipelineLayout(const PipelineLayout &) = delete;
	PipelineLayout(PipelineLayout &&other);

	~PipelineLayout();

	PipelineLayout &operator=(const PipelineLayout &) = delete;
	PipelineLayout &operator=(PipelineLayout &&)      = delete;

	vk::PipelineLayout get_handle() const;

	std::vector<backend::ShaderResource> get_resources(
	    const backend::ShaderResourceType &type  = backend::ShaderResourceType::kAll,
	    vk::ShaderStageFlags               stage = vk::ShaderStageFlagBits::eAll) const;

	const std::vector<ShaderModule *> &get_shader_modules() const;

  private:
	Device            &device_;
	vk::PipelineLayout handle_{VK_NULL_HANDLE};

	std::vector<ShaderModule *> shader_modules_;

	std::unordered_map<std::string, ShaderResource> shader_resources_;

	std::unordered_map<uint32_t, std::vector<ShaderResource>> shader_sets_;

	std::vector<DescriptorSetLayout *> descriptor_set_layouts_;
};
}        // namespace xihe::backend