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
    queue_family_properties_{physical_device.getQueueFamilyProperties()},
	device_extensions_{physical_device.enumerateDeviceExtensionProperties()}
{
	LOGI("Found GPU: {}", properties_.deviceName.data());

	// Display supported extensions
	if (device_extensions_.size() > 0)
	{
		LOGD("Device supports the following extensions:");
		for (auto &extension : device_extensions_)
		{
			LOGD("  \t{}", extension.extensionName.data());
		}
	}
}

void * PhysicalDevice::get_extension_feature_chain() const
{
	return last_requested_extension_feature_;
}

bool PhysicalDevice::is_extension_supported(const std::string &requested_extension) const
{
	return std::ranges::find_if(
	           device_extensions_,
	           [requested_extension](auto &ext) { return std::strcmp(ext.extensionName, requested_extension.c_str()) == 0; }) != device_extensions_.end();
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

vk::PhysicalDeviceFeatures PhysicalDevice::get_features() const
{
	return features_;
}

vk::PhysicalDeviceFeatures PhysicalDevice::get_requested_features() const
{
	return requested_features_;
}

vk::PhysicalDeviceFeatures &PhysicalDevice::get_mutable_requested_features()
{
	return requested_features_;
}

vk::FormatProperties PhysicalDevice::get_format_properties(vk::Format format) const
{
	return handle_.getFormatProperties(format);
}

bool PhysicalDevice::has_high_priority_graphics_queue() const
{
	return high_priority_graphics_queue_;
}
}        // namespace backend
}        // namespace xihe
