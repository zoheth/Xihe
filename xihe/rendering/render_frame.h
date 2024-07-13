#pragma once

#include <vulkan/vulkan_hash.hpp>

#include "backend/device.h"
#include "backend/buffer_pool.h"
#include "backend/descriptor_pool.h"
#include "backend/descriptor_set.h"
#include "backend/semaphore_pool.h"
#include "rendering/render_target.h"

namespace xihe::rendering
{
enum class BufferAllocationStrategy
{
	kOneAllocationPerBuffer,
	kMultipleAllocationsPerBuffer
};

enum class DescriptorManagementStrategy
{
	kStoreInCache,
	kCreateDirectly
};
;
class RenderFrame
{
  public:
	RenderFrame(backend::Device &device, size_t thread_count = 1);

	RenderFrame(const RenderFrame &)            = delete;
	RenderFrame(RenderFrame &&)                 = delete;
	RenderFrame &operator=(const RenderFrame &) = delete;
	RenderFrame &operator=(RenderFrame &&)      = delete;

	backend::CommandBuffer &request_command_buffer(const backend::Queue &queue, backend::CommandBuffer::ResetMode reset_mode,
		vk::CommandBufferLevel level,
		size_t thread_index);

	RenderTarget &      get_render_target(const std::string &rdg_name);
	RenderTarget const &get_render_target(const std::string &rdg_name) const;

	vk::DescriptorSet request_descriptor_set(const backend::DescriptorSetLayout    &descriptor_set_layout,
	                                         const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
	                                         const BindingMap<vk::DescriptorImageInfo>  &image_infos,
	                                         bool                                        update_after_bind,
	                                         size_t                                      thread_index);

	vk::Fence     request_fence();
	vk::Semaphore request_semaphore();
	vk::Semaphore request_semaphore_with_ownership();
	void          reset();

	void clear_descriptors();

	void update_render_target(std::string rdg_name, std::unique_ptr<RenderTarget> &&render_target);

	void release_owned_semaphore(vk::Semaphore semaphore);

	backend::BufferAllocation allocate_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index);

private:
	std::vector<std::unique_ptr<backend::CommandPool>> &get_command_pools(const backend::Queue &queue, backend::CommandBuffer::ResetMode reset_mode);

	static std::vector<uint32_t> collect_bindings_to_update(const backend::DescriptorSetLayout    &descriptor_set_layout,
	                                                        const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
	                                                        const BindingMap<vk::DescriptorImageInfo>  &image_infos);
  private:
	const std::unordered_map<vk::BufferUsageFlags, uint32_t> supported_usage_map_ = {
	    {vk::BufferUsageFlagBits::eUniformBuffer, 1},
	    {vk::BufferUsageFlagBits::eStorageBuffer, 2},        // x2 the size of BUFFER_POOL_BLOCK_SIZE since SSBOs are normally much larger than other types of buffers
	    {vk::BufferUsageFlagBits::eVertexBuffer, 1},
	    {vk::BufferUsageFlagBits::eIndexBuffer, 1}};

	backend::Device &device_;

	std::unordered_map<std::string, std::unique_ptr<RenderTarget>> render_targets_ = {};

	/// Commands pools associated to the frame
	std::map<uint32_t, std::vector<std::unique_ptr<backend::CommandPool>>> command_pools_;

	/// Descriptor pools for the frame
	std::vector<std::unique_ptr<std::unordered_map<std::size_t, backend::DescriptorPool>>> descriptor_pools_;

	/// Descriptor sets for the frame
	std::vector<std::unique_ptr<std::unordered_map<std::size_t, backend::DescriptorSet>>> descriptor_sets_;

	backend::FencePool fence_pool_;

	backend::SemaphorePool semaphore_pool_;

	size_t thread_count_;

	BufferAllocationStrategy buffer_allocation_strategy_{BufferAllocationStrategy::kMultipleAllocationsPerBuffer};

	DescriptorManagementStrategy descriptor_management_strategy_{DescriptorManagementStrategy::kStoreInCache};

	std::map<vk::BufferUsageFlags, std::vector<std::pair<backend::BufferPool, backend::BufferBlock *>>> buffer_pools_;
};
}        // namespace xihe::rendering
