#include "physical_device.h"

#include "common/logging.h"

namespace xihe
{
namespace backend
{
PhysicalDevice::PhysicalDevice(Instance &instance, vk::PhysicalDevice physical_device) :
    instance_{instance},
    handle_{physical_device},
    features_{physical_device.getFeatures()},
    properties_{physical_device.getProperties()},
    memory_properties_{physical_device.getMemoryProperties()},
    queue_family_properties_{physical_device.getQueueFamilyProperties()}
{
	LOGI("Found GPU: {}", properties_.deviceName.data());
}

vk::PhysicalDevice PhysicalDevice::get_handle() const
{
	return handle_;
}

Instance & PhysicalDevice::get_instance() const
{
	return instance_;
}

const vk::PhysicalDeviceProperties &PhysicalDevice::get_properties() const
{
	return properties_;
}

const std::vector<vk::QueueFamilyProperties> &PhysicalDevice::get_queue_family_properties() const
{
	return queue_family_properties_;
}

vk::PhysicalDeviceFeatures PhysicalDevice::get_requested_features() const
{
	return requested_features_;
}

vk::PhysicalDeviceFeatures &PhysicalDevice::get_mutable_requested_features()
{
	return requested_features_;
}

bool PhysicalDevice::has_high_priority_graphics_queue() const
{
	return high_priority_graphics_queue_;
}
}        // namespace backend
}        // namespace xihe
