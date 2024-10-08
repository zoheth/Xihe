#include "device.h"

#include <volk.h>

#include "common/error.h"
#include "common/logging.h"

#include "backend/resources_management/resource_cache.h"

XH_DISABLE_WARNINGS()
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
XH_ENABLE_WARNINGS()

namespace xihe
{
namespace backend
{
Device::Device(PhysicalDevice                        &gpu,
               vk::SurfaceKHR                         surface,
               std::unique_ptr<DebugUtils>          &&debug_utils,
               std::unordered_map<const char *, bool> requested_extensions) :
    VulkanResource{nullptr, this},
    gpu_{gpu},
    resource_cache_(*this),
    debug_utils_(std::move(debug_utils))
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
		auto perf_counter_features     = gpu.get_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
		auto host_query_reset_features = gpu.get_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>();

		if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset)
		{
			gpu.add_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>().performanceCounterQueryPools = true;
			gpu.add_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>().hostQueryReset                    = true;
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

	create_info.pNext = gpu.get_extension_feature_chain();

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

	backend::allocated::init(*this);

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

ResourceCache &Device::get_resource_cache()
{
	return resource_cache_;
}

CommandBuffer & Device::request_command_buffer() const
{
	return command_pool_->request_command_buffer();
}

vk::Fence Device::request_fence() const
{

	return fence_pool_->request_fence();
}

void Device::wait_idle() const
{
	get_handle().waitIdle();
}

FencePool & Device::get_fence_pool() const
{
	return *fence_pool_;
}

CommandPool &Device::get_command_pool() const
{
	return *command_pool_;
}

Device::~Device()
{
	resource_cache_.clear();

	command_pool_.reset();
	fence_pool_.reset();

	allocated::shutdown();

	if (get_handle())
	{
		get_handle().destroy();
	}
}

bool Device::is_extension_supported(std::string const &extension) const
{
	return gpu_.is_extension_supported(extension);
}

bool Device::is_enabled(std::string const &extension) const
{
	return std::ranges::find_if(
	           enabled_extensions_,
	           [extension](const char *enabled_extension) { return extension == enabled_extension; }) != enabled_extensions_.end();
}

bool Device::is_image_format_supported(vk::Format format) const
{
	vk::ImageFormatProperties image_format_properties;

	const auto result = gpu_.get_handle().getImageFormatProperties(format, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled, vk::ImageCreateFlags(), &image_format_properties);

	return result != vk::Result::eErrorFormatNotSupported;
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
