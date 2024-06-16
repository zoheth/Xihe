#pragma once

#include "shader_module.h"

#include <map>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;
class ShaderModule;
class DescriptorSetLayout;
class BindlessDescriptorSet;

class PipelineLayout
{
  public:
	PipelineLayout(Device &device, const std::vector<ShaderModule *> &shader_modules, BindlessDescriptorSet *bindless_descriptor_set = nullptr);

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

	vk::ShaderStageFlags get_push_constant_range_stage(uint32_t size, uint32_t offset = 0) const;

	DescriptorSetLayout const                                       &get_descriptor_set_layout(const uint32_t set_index) const;
	const std::map<uint32_t, std::vector<ShaderResource>> &get_shader_sets() const;
	bool                                                             has_descriptor_set_layout(const uint32_t set_index) const;

	vk::DescriptorSet get_bindless_descriptor_set() const;

	uint32_t get_bindless_descriptor_set_index() const;

private:
	void aggregate_shader_resources();

  private:
	Device            &device_;
	vk::PipelineLayout handle_{VK_NULL_HANDLE};

	std::vector<ShaderModule *> shader_modules_;

	std::unordered_map<std::string, ShaderResource> shader_resources_;

	std::map<uint32_t, std::vector<ShaderResource>> shader_sets_;

	std::vector<DescriptorSetLayout *> descriptor_set_layouts_;

	BindlessDescriptorSet *bindless_descriptor_set_{nullptr};
	uint32_t 			 bindless_descriptor_set_index_{0};
};
}        // namespace xihe::backend