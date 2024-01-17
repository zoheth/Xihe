#include "device.h"

#include "common/logging.h"

namespace xihe
{
namespace backend
{
Device::Device(PhysicalDevice &gpu, vk::SurfaceKHR surface, std::unique_ptr<DebugUtils> &&debug_utils, std::unordered_map<const char *, bool> requested_extensions) :
    VulkanResource{nullptr, this},
    gpu_{gpu},
    resource_cache_(*this)
{
	LOGI("Selected GPU: {}", gpu.get_properties().deviceName.data());

    // Prepare the device queues
	std::vector<vk::QueueFamilyProperties> queue_family_properties = gpu.get_queue_family_properties();
	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos(queue_family_properties.size());
	std::vector<std::vector<float>>        queue_priorities(queue_family_properties.size());

	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
	{
		vk::QueueFamilyProperties const &queue_family_property = queue_family_properties[queue_family_index];

		if (gpu.has_high_priority_graphics_queue())
		{
			uint32_t graphics_queue_family = get_queue_family_index(vk::QueueFlagBits::eGraphics);
			if (graphics_queue_family == queue_family_index)
			{
				queue_priorities[queue_family_index].reserve(queue_family_property.queueCount);
				queue_priorities[queue_family_index].push_back(1.0f);
				for (uint32_t i = 1; i < queue_family_property.queueCount; i++)
				{
					queue_priorities[queue_family_index].push_back(0.5f);
				}
			}
			else
			{
				queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
			}
		}
		else
		{
			queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
		}

		vk::DeviceQueueCreateInfo &queue_create_info = queue_create_infos[queue_family_index];

		queue_create_info.queueFamilyIndex = queue_family_index;
		queue_create_info.queueCount       = queue_family_property.queueCount;
		queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
	}
}

DebugUtils const &Device::get_debug_utils() const
{
	return *debug_utils_;
}
}        // namespace backend
}        // namespace xihe
