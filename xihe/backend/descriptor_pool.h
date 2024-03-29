#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;
class DescriptorSetLayout;

class DescriptorPool
{
  public:
	static constexpr uint32_t kMaxSetsPerPool = 16;

	DescriptorPool(Device &device, const DescriptorSetLayout &descriptor_set_layout, uint32_t pool_size = kMaxSetsPerPool);

  private:
	Device &device_;

	const DescriptorSetLayout *descriptor_set_layout_{nullptr};

	std::vector<vk::DescriptorPoolSize> pool_sizes_;
	std::vector<vk::DescriptorPool>     pools_;
	std::vector<uint32_t>               pool_sets_count_;

	uint32_t pool_max_sets_{0};
	uint32_t pool_index_{0};

	std::unordered_map<VkDescriptorSet, uint32_t> set_pool_mapping_;
};
}        // namespace xihe::backend
