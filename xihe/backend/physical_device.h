#pragma once

#include <map>

#include <vulkan/vulkan.hpp>

#include "backend/instance.h"

namespace xihe
{
namespace backend
{

class PhysicalDevice
{
  public:
	PhysicalDevice(Instance &instance, vk::PhysicalDevice physical_device);

	PhysicalDevice(const PhysicalDevice &)            = delete;
	PhysicalDevice(PhysicalDevice &&)                 = delete;
	PhysicalDevice &operator=(const PhysicalDevice &) = delete;
	PhysicalDevice &operator=(PhysicalDevice &&)      = delete;

	vk::PhysicalDevice get_handle() const;

	const vk::PhysicalDeviceProperties           &get_properties() const;
	const std::vector<vk::QueueFamilyProperties> &get_queue_family_properties() const;
	vk::PhysicalDeviceFeatures                    get_requested_features() const;
	vk::PhysicalDeviceFeatures                   &get_mutable_requested_features();

  private:
	Instance &instance_;

	vk::PhysicalDevice handle_{VK_NULL_HANDLE};

	vk::PhysicalDeviceFeatures         features_;
	vk::PhysicalDeviceProperties       properties_;
	vk::PhysicalDeviceMemoryProperties memory_properties_;

	// The GPU queue family properties
	std::vector<vk::QueueFamilyProperties> queue_family_properties_;

	// The features that will be requested to be enabled in the logical device
	vk::PhysicalDeviceFeatures requested_features_;

	void *last_requested_extension_feature_{nullptr};

	// Holds the extension feature structures, we use a map to retain an order of requested structures
	std::map<vk::StructureType, std::shared_ptr<void>> extension_features_;

	bool high_priority_graphics_queue_{false};
};
}        // namespace backend
}        // namespace xihe