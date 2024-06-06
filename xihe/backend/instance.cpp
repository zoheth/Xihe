#include "instance.h"

#include <volk.h>

#include "backend/physical_device.h"
#include "common/logging.h"

#if defined(XH_DEBUG) || defined(XH_VALIDATION_LAYERS)
#	define USE_VALIDATION_LAYERS
#endif

namespace xihe
{
namespace
{
#ifdef USE_VALIDATION_LAYERS
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                              void                                       *user_data)
{
	// Log debug message
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
                                              uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/,
                                              const char *layer_prefix, const char *message, void * /*user_data*/)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		LOGE("{}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else
	{
		LOGI("{}: {}", layer_prefix, message);
	}
	return VK_FALSE;
}
#endif

bool validate_layers(const std::vector<const char *>        &required,
                     const std::vector<vk::LayerProperties> &available)
{
	for (auto layer : required)
	{
		bool found = false;
		for (auto &available_layer : available)
		{
			if (strcmp(available_layer.layerName, layer) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			LOGE("Validation Layer {} not found", layer);
			return false;
		}
	}

	return true;
}

bool enable_extension(const char                                 *required_ext_name,
                      const std::vector<vk::ExtensionProperties> &available_exts,
                      std::vector<const char *>                  &enabled_extensions)
{
	for (auto &avail_ext_it : available_exts)
	{
		if (strcmp(avail_ext_it.extensionName, required_ext_name) == 0)
		{
			auto it = std::find_if(enabled_extensions.begin(), enabled_extensions.end(),
			                       [required_ext_name](const char *enabled_ext_name) {
				                       return strcmp(enabled_ext_name, required_ext_name) == 0;
			                       });
			if (it != enabled_extensions.end())
			{
				// Extension is already enabled
			}
			else
			{
				LOGI("Extension {} found, enabling it", required_ext_name);
				enabled_extensions.emplace_back(required_ext_name);
			}
			return true;
		}
	}

	LOGI("Extension {} not found", required_ext_name);
	return false;
}

}        // namespace

namespace backend
{
std::vector<const char *> get_optimal_validation_layers(const std::vector<vk::LayerProperties> &supported_instance_layers)
{
	std::vector<std::vector<const char *>> validation_layer_priority_list =
	    {
	        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
	        {"VK_LAYER_KHRONOS_validation"},

	        // Otherwise we fallback to using the LunarG meta layer
	        {"VK_LAYER_LUNARG_standard_validation"},

	        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
	        {
	            "VK_LAYER_GOOGLE_threading",
	            "VK_LAYER_LUNARG_parameter_validation",
	            "VK_LAYER_LUNARG_object_tracker",
	            "VK_LAYER_LUNARG_core_validation",
	            "VK_LAYER_GOOGLE_unique_objects",
	        },

	        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
	        {"VK_LAYER_LUNARG_core_validation"}};

	for (auto &validation_layers : validation_layer_priority_list)
	{
		if (validate_layers(validation_layers, supported_instance_layers))
		{
			return validation_layers;
		}

		LOGW("Couldn't enable validation layers (see log for error) - falling back");
	}

	// Else return nothing
	return {};
}

Instance::Instance(const std::string &application_name, const std::unordered_map<const char *, bool> &required_extensions, const std::vector<const char *> &required_validation_layers, uint32_t api_version)
{
	std::vector<vk::ExtensionProperties> available_extensions = vk::enumerateInstanceExtensionProperties();

#ifdef USE_VALIDATION_LAYERS
	// Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
	const bool has_debug_utils = enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	                                              available_extensions, enabled_extensions_);

	if (!has_debug_utils)
	{
		if (const bool has_debug_report = enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		                                                   available_extensions, enabled_extensions_);
		    !has_debug_report)
		{
			LOGW("Neither of {} or {} are available; disabling debug reporting",
			     VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
	}
#endif

	enabled_extensions_.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	// VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
	// which will be used for stats gathering where available.
	enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	                 available_extensions, enabled_extensions_);

	auto extension_error = false;
	for (const auto extension : required_extensions)
	{
		auto extension_name        = extension.first;
		auto extension_is_optional = extension.second;
		if (!enable_extension(extension_name, available_extensions, enabled_extensions_))
		{
			if (extension_is_optional)
			{
				LOGW("Optional instance extension {} not available, some features may be disabled", extension_name);
			}
			else
			{
				LOGE("Required instance extension {} not available, cannot run", extension_name);
				extension_error = true;
			}
			extension_error = extension_error || !extension_is_optional;
		}
	}

	if (extension_error)
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}

	std::vector<vk::LayerProperties> supported_validation_layers = vk::enumerateInstanceLayerProperties();

	std::vector<const char *> requested_validation_layers(required_validation_layers);

#ifdef USE_VALIDATION_LAYERS
	// Determine the optimal validation layers to enable that are necessary for useful debugging
	std::vector<const char *> optimal_validation_layers = get_optimal_validation_layers(supported_validation_layers);
	requested_validation_layers.insert(requested_validation_layers.end(), optimal_validation_layers.begin(), optimal_validation_layers.end());
#endif

	if (validate_layers(requested_validation_layers, supported_validation_layers))
	{
		LOGI("Enabled Validation Layers:");
		for (const auto &layer : requested_validation_layers)
		{
			LOGI("	\t{}", layer);
		}
	}
	else
	{
		throw std::runtime_error("Required validation layers are missing.");
	}

	vk::ApplicationInfo app_info(application_name.c_str(), 0, "Xihe", 0, api_version);

	vk::InstanceCreateInfo instance_info({}, &app_info, requested_validation_layers, enabled_extensions_);

#ifdef USE_VALIDATION_LAYERS
	vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info;
	vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;
	if (has_debug_utils)
	{
		debug_utils_create_info =
		    vk::DebugUtilsMessengerCreateInfoEXT({},
		                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
		                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		                                         debug_utils_messenger_callback);

		instance_info.pNext = &debug_utils_create_info;
	}
	else
	{
		debug_report_create_info = vk::DebugReportCallbackCreateInfoEXT(
		    vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning, debug_callback);

		instance_info.pNext = &debug_report_create_info;
	}
#endif

	// Create the Vulkan instance
	handle_ = vk::createInstance(instance_info);

	// initialize the Vulkan-Hpp default dispatcher on the instance
	VULKAN_HPP_DEFAULT_DISPATCHER.init(handle_);

	// Need to load volk for all the not-yet Vulkan-Hpp calls
	volkLoadInstance(handle_);

#ifdef USE_VALIDATION_LAYERS
	if (has_debug_utils)
	{
		debug_utils_messenger_ = handle_.createDebugUtilsMessengerEXT(debug_utils_create_info);
	}
	else
	{
		debug_report_callback_ = handle_.createDebugReportCallbackEXT(debug_report_create_info);
	}
#endif

	query_gpus();
}

Instance::Instance(vk::Instance instance) :
    handle_(instance)
{
	if (handle_)
	{
		query_gpus();
	}
	else
	{
		throw std::runtime_error("Invalid Vulkan instance.");
	}
}

Instance::~Instance()
{
#ifdef USE_VALIDATION_LAYERS
	if (debug_utils_messenger_)
	{
		handle_.destroyDebugUtilsMessengerEXT(debug_utils_messenger_);
	}
	if (debug_report_callback_)
	{
		handle_.destroyDebugReportCallbackEXT(debug_report_callback_);
	}
#endif

	if (handle_)
	{
		handle_.destroy();
	}
}

vk::Instance Instance::get_handle() const
{
	return handle_;
}

PhysicalDevice &Instance::get_suitable_gpu(vk::SurfaceKHR surface) const
{
	assert(!gpus_.empty() && "No physical devices were found on the system.");

	// Find a discrete GPU
	for (auto &gpu : gpus_)
	{
		if (gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			// See if it work with the surface
			size_t queue_count = gpu->get_queue_family_properties().size();
			for (uint32_t queue_idx = 0; static_cast<size_t>(queue_idx) < queue_count; queue_idx++)
			{
				if (gpu->get_handle().getSurfaceSupportKHR(queue_idx, surface))
				{
					return *gpu;
				}
			}
		}
	}

	// Otherwise just pick the first one
	LOGW("Couldn't find a discrete physical device, picking default GPU");
	return *gpus_[0];
}

bool Instance::is_enabled(const char *extension) const
{
	return std::ranges::find_if(
	           enabled_extensions_,
	           [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) != enabled_extensions_.end();
}

void Instance::query_gpus()
{
	// Querying valid physical devices on the machine
	std::vector<vk::PhysicalDevice> physical_devices = handle_.enumeratePhysicalDevices();
	if (physical_devices.empty())
	{
		throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
	}

	// Create gpus wrapper objects from the vk::PhysicalDevice's
	for (auto &physical_device : physical_devices)
	{
		gpus_.push_back(std::make_unique<PhysicalDevice>(*this, physical_device));
	}
}
}        // namespace backend
}        // namespace xihe
