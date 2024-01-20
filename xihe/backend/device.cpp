#include "device.h"

#include <volk.h>

#include "common/error.h"
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

	// Check extensions to enable Vma Dedicated Allocation
	device_extensions_ = gpu.get_handle().enumerateDeviceExtensionProperties();

	// Display supported extensions
	if (!device_extensions_.empty())
	{
		LOGD("Device extensions supported by GPU:");
		for (auto &extension : device_extensions_)
		{
			LOGD("  \t{}", extension.extensionName.data());
		}
	}

	bool can_get_memory_requirements = is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	bool has_dedicated_allocation    = is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

	if (can_get_memory_requirements && has_dedicated_allocation)
	{
		enabled_extensions_.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
		enabled_extensions_.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

		LOGI("Dedicated Allocation enabled");
	}

	// For performance queries, we also use host query reset since queryPool resets cannot
	// live in the same command buffer as beginQuery
	if (is_extension_supported(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME) && is_extension_supported(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME))
	{
		auto perf_counter_features     = gpu.request_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
		auto host_query_reset_features = gpu.request_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>();

		if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset)
		{
			enabled_extensions_.push_back(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
			enabled_extensions_.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
			LOGI("Performance query enabled");
		}
	}

	// Check that extensions are supported before trying to create the device
	std::vector<const char *> unsupported_extensions{};
	for (auto &extension : requested_extensions)
	{
		if (is_extension_supported(extension.first))
		{
			enabled_extensions_.emplace_back(extension.first);
		}
		else
		{
			unsupported_extensions.emplace_back(extension.first);
		}
	}

	if (!enabled_extensions_.empty())
	{
		LOGI("Device supports the following requested extensions:");
		for (auto &extension : enabled_extensions_)
		{
			LOGI("  \t{}", extension);
		}
	}

	if (!unsupported_extensions.empty())
	{
		auto error = false;
		for (auto &extension : unsupported_extensions)
		{
			if (auto extension_is_optional = requested_extensions[extension])
			{
				LOGW("Optional device extension {} not available, some features may be disabled", extension);
			}
			else
			{
				LOGE("Required device extension {} not available, cannot run", extension);
				error = true;
			}
		}

		if (error)
		{
			throw VulkanException(vk::Result::eErrorExtensionNotPresent, "Extensions not present");
		}
	}

	vk::DeviceCreateInfo create_info({}, queue_create_infos, {}, enabled_extensions_, &gpu.get_mutable_requested_features());

	create_info.pNext = &gpu.get_mutable_requested_features();

	set_handle(gpu_.get_handle().createDevice(create_info));

	queues_.resize(queue_family_properties.size());

	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
	{
		vk::QueueFamilyProperties const &queue_family_property = queue_family_properties[queue_family_index];

		vk::Bool32 present_supported = gpu.get_handle().getSurfaceSupportKHR(queue_family_index, surface);

		for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index)
		{
			queues_[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property, present_supported, queue_index);
		}
	}

	VmaVulkanFunctions vma_vulkan_func{};
	vma_vulkan_func.vkAllocateMemory                    = reinterpret_cast<PFN_vkAllocateMemory>(get_handle().getProcAddr("vkAllocateMemory"));
	vma_vulkan_func.vkBindBufferMemory                  = reinterpret_cast<PFN_vkBindBufferMemory>(get_handle().getProcAddr("vkBindBufferMemory"));
	vma_vulkan_func.vkBindImageMemory                   = reinterpret_cast<PFN_vkBindImageMemory>(get_handle().getProcAddr("vkBindImageMemory"));
	vma_vulkan_func.vkCreateBuffer                      = reinterpret_cast<PFN_vkCreateBuffer>(get_handle().getProcAddr("vkCreateBuffer"));
	vma_vulkan_func.vkCreateImage                       = reinterpret_cast<PFN_vkCreateImage>(get_handle().getProcAddr("vkCreateImage"));
	vma_vulkan_func.vkDestroyBuffer                     = reinterpret_cast<PFN_vkDestroyBuffer>(get_handle().getProcAddr("vkDestroyBuffer"));
	vma_vulkan_func.vkDestroyImage                      = reinterpret_cast<PFN_vkDestroyImage>(get_handle().getProcAddr("vkDestroyImage"));
	vma_vulkan_func.vkFlushMappedMemoryRanges           = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(get_handle().getProcAddr("vkFlushMappedMemoryRanges"));
	vma_vulkan_func.vkFreeMemory                        = reinterpret_cast<PFN_vkFreeMemory>(get_handle().getProcAddr("vkFreeMemory"));
	vma_vulkan_func.vkGetBufferMemoryRequirements       = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(get_handle().getProcAddr("vkGetBufferMemoryRequirements"));
	vma_vulkan_func.vkGetImageMemoryRequirements        = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(get_handle().getProcAddr("vkGetImageMemoryRequirements"));
	vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(get_handle().getProcAddr("vkGetPhysicalDeviceMemoryProperties"));
	vma_vulkan_func.vkGetPhysicalDeviceProperties       = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(get_handle().getProcAddr("vkGetPhysicalDeviceProperties"));
	vma_vulkan_func.vkInvalidateMappedMemoryRanges      = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(get_handle().getProcAddr("vkInvalidateMappedMemoryRanges"));
	vma_vulkan_func.vkMapMemory                         = reinterpret_cast<PFN_vkMapMemory>(get_handle().getProcAddr("vkMapMemory"));
	vma_vulkan_func.vkUnmapMemory                       = reinterpret_cast<PFN_vkUnmapMemory>(get_handle().getProcAddr("vkUnmapMemory"));
	vma_vulkan_func.vkCmdCopyBuffer                     = reinterpret_cast<PFN_vkCmdCopyBuffer>(get_handle().getProcAddr("vkCmdCopyBuffer"));

	VmaAllocatorCreateInfo allocator_info{};
	allocator_info.physicalDevice = static_cast<VkPhysicalDevice>(gpu.get_handle());
	allocator_info.device         = static_cast<VkDevice>(get_handle());
	allocator_info.instance       = static_cast<VkInstance>(gpu.get_instance().get_handle());

	if (can_get_memory_requirements && has_dedicated_allocation)
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
		vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
		vma_vulkan_func.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR;
	}

	if (is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	allocator_info.pVulkanFunctions = &vma_vulkan_func;

	VkResult result = vmaCreateAllocator(&allocator_info, &memory_allocator_);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create allocator"};
	}

	command_pool_ = std::make_unique<CommandPool>(*this, get_queue_by_flags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index());
	fence_pool_   = std::make_unique<FencePool>(*this);
}

uint32_t Device::get_queue_family_index(vk::QueueFlagBits queue_flag) const
{
	const auto &queue_family_properties = gpu_.get_queue_family_properties();

	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if (queue_flag & vk::QueueFlagBits::eCompute)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
		{
			if ((queue_family_properties[i].queueFlags & queue_flag) && !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics))
			{
				return i;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queue_flag & vk::QueueFlagBits::eTransfer)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
		{
			if ((queue_family_properties[i].queueFlags & queue_flag) && !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
			    !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eCompute))
			{
				return i;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested
	// flags
	for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
	{
		if (queue_family_properties[i].queueFlags & queue_flag)
		{
			return i;
		}
	}

	throw std::runtime_error("Could not find a matching queue family index");
}

Queue const &Device::get_queue(uint32_t queue_family_index, uint32_t queue_index) const
{
	return queues_[queue_family_index][queue_index];
}

Queue const &Device::get_queue_by_flags(vk::QueueFlags required_queue_flags, uint32_t queue_index) const
{
	for (const auto &queue : queues_)
	{
		Queue const &first_queue = queue[0];

		vk::QueueFlags queue_flags = first_queue.get_properties().queueFlags;
		uint32_t       queue_count = first_queue.get_properties().queueCount;

		if (((queue_flags & required_queue_flags) == required_queue_flags) && queue_index < queue_count)
		{
			return queue[queue_index];
		}
	}

	throw std::runtime_error("Could not find a matching queue family index");
}

Queue const &Device::get_suitable_graphics_queue() const
{
	for (const auto &queue : queues_)
	{
		Queue const &first_queue = queue[0];

		const uint32_t queue_count = first_queue.get_properties().queueCount;

		if (first_queue.support_present() && queue_count > 0)
		{
			return queue[0];
		}
	}

	return get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
}

bool Device::is_extension_supported(std::string const &extension) const
{
	return std::ranges::find_if(
	           device_extensions_,
	           [extension](auto &ext) { return std::strcmp(ext.extensionName, extension.c_str()) == 0; }) != device_extensions_.end();
}

bool Device::is_enabled(std::string const &extension) const
{
	return std::ranges::find_if(
	           enabled_extensions_,
	           [extension](const char *enabled_extension) { return extension == enabled_extension; }) != enabled_extensions_.end();
}

PhysicalDevice const &Device::get_gpu() const
{
	return gpu_;
}

DebugUtils const &Device::get_debug_utils() const
{
	return *debug_utils_;
}
}        // namespace backend
}        // namespace xihe
