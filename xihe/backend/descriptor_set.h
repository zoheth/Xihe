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

	/**
	 * @brief Updates the contents of the DescriptorSet by performing the write operations
	 * @param bindings_to_update If empty. we update all bindings. Otherwise, only write the specified bindings if they haven't already been written
	 */
	void update(const std::vector<uint32_t> &bindings_to_update = {});

	void apply_writes() const;

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

	DescriptorPool &get_descriptor_pool()
	{
		return descriptor_pool_;
	}

  private:
	/**
	 * \brief Prepares the descriptor set to have its contents updated by loading a vector of write operations
	 *		  Cannot be called twice during the lifetime of a DescriptorSet
	 */
	void prepare();

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

class BindlessDescriptorSet
{
  public:
	explicit BindlessDescriptorSet(Device &device);

	~BindlessDescriptorSet();

	vk::DescriptorSet get_handle() const;

	vk::DescriptorSetLayout get_layout() const;

	void update(uint32_t index, vk::DescriptorImageInfo image_info);

	uint32_t update(vk::DescriptorImageInfo image_info);

	uint32_t update(vk::DescriptorBufferInfo buffer_info);

	void reset_index();

	static constexpr uint32_t bindless_texture_binding_ = 10;
	static constexpr uint32_t bindless_buffer_binding_  = 11;

	static constexpr uint32_t max_bindless_resources_   = 1024;

  private:
	Device                 &device_;
	vk::DescriptorSet       handle_{VK_NULL_HANDLE};
	vk::DescriptorPool      descriptor_pool_;
	vk::DescriptorSetLayout bindless_descriptor_set_layout_;

	uint32_t next_image_index_ = 0;
	uint32_t next_buffer_index_ = 0;
};

}        // namespace xihe::backend
