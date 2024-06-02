#pragma once

#include <vk_mem_alloc.h>

#include "common/error.h"
#include "backend/command_pool.h"
#include "backend/debug.h"
#include "backend/fence_pool.h"
#include "backend/physical_device.h"
#include "backend/queue.h"
#include "backend/resources_management/resource_cache.h"
#include "backend/vulkan_resource.h"

namespace xihe
{
namespace backend
{
class Device : public backend::VulkanResource<vk::Device>
{
  public:
	Device(PhysicalDevice                        &gpu,
	       vk::SurfaceKHR                         surface,
	       std::unique_ptr<DebugUtils>          &&debug_utils,
	       std::unordered_map<const char *, bool> requested_extensions = {});

	Device(const Device &)            = delete;
	Device(Device &&)                 = delete;
	Device &operator=(const Device &) = delete;
	Device &operator=(Device &&)      = delete;

	~Device();

	bool is_extension_supported(std::string const &extension) const;
	bool is_enabled(std::string const &extension) const;

	bool is_image_format_supported(vk::Format format) const;

	PhysicalDevice const &get_gpu() const;

	DebugUtils const &get_debug_utils() const;

	uint32_t get_queue_family_index(vk::QueueFlagBits queue_flags) const;

	Queue const &get_queue(uint32_t queue_family_index, uint32_t queue_index) const;
	Queue const &get_queue_by_flags(vk::QueueFlags required_queue_flags, uint32_t queue_index) const;

	/**
	 * \brief 
	 * \return The first present supported queue
	 */
	Queue const &get_suitable_graphics_queue() const;

	ResourceCache &get_resource_cache();

	CommandBuffer &request_command_buffer() const;

	vk::Fence request_fence() const;

	void wait_idle() const;

	FencePool &get_fence_pool() const;

	CommandPool &get_command_pool() const;

  private:
	PhysicalDevice const &gpu_;
	vk::SurfaceKHR        surface_{nullptr};
	ResourceCache         resource_cache_;

	// std::vector<vk::ExtensionProperties> device_extensions_;
	std::vector<const char *>            enabled_extensions_;

	std::vector<std::vector<Queue>> queues_;

	std::unique_ptr<DebugUtils> debug_utils_{nullptr};

	std::unique_ptr<CommandPool> command_pool_{nullptr};
	std::unique_ptr<FencePool>   fence_pool_{nullptr};
};
}        // namespace backend
}        // namespace xihe