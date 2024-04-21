#pragma once
#include "common/vk_common.h"

namespace xihe::backend
{
class Device;
class DescriptorSetLayout;
class DescriptorPool;

class DescriptorSet
{
  public:
	DescriptorSet(Device                                     &device,
	              const DescriptorSetLayout                  &descriptor_set_layout,
	              DescriptorPool                             &descriptor_pool,
	              const BindingMap<vk::DescriptorBufferInfo> &buffer_infos = {},
	              const BindingMap<vk::DescriptorImageInfo>  &image_infos  = {});

	const DescriptorSetLayout &get_layout() const;

	vk::DescriptorSet get_handle() const;

	BindingMap<vk::DescriptorBufferInfo> &get_buffer_infos()
	{
		return buffer_infos_;
	}

	BindingMap<vk::DescriptorImageInfo> &get_image_infos()
	{
		return image_infos_;
	}

private:
	Device &device_;

	const DescriptorSetLayout &descriptor_set_layout_;

	DescriptorPool &descriptor_pool_;

	BindingMap<vk::DescriptorBufferInfo> buffer_infos_;

	BindingMap<vk::DescriptorImageInfo> image_infos_;

	vk::DescriptorSet handle_{VK_NULL_HANDLE};

	// The list of write operations for the descriptor set
	std::vector<vk::WriteDescriptorSet> write_descriptor_sets_;

	// The bindings of the write descriptors that had vkUpdateDescriptorSets since the last call to update().
	// Each binding number is mapped to a hash of the binding description that it will be updated to.
	std::unordered_map<uint32_t, size_t> updated_bindings_;
};
}        // namespace xihe::backend
