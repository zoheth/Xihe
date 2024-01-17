#pragma once

#include "backend/debug.h"
#include "backend/physical_device.h"
#include "backend/vulkan_resource.h"
#include "backend/resource_cache.h"

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

	Device(const Device &) = delete;
	Device(Device &&) = delete;
	Device &operator=(const Device &) = delete;
	Device &operator=(Device &&) = delete;

	~Device();

	//PhysicalDevice const &get_gpu() const;



	DebugUtils const &get_debug_utils() const;

  private:
	PhysicalDevice const &gpu_;
	vk::SurfaceKHR surface_{nullptr};
	ResourceCache resource_cache_;


	std::unique_ptr<DebugUtils> debug_utils_{nullptr};

};
}        // namespace backend
}        // namespace xihe