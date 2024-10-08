#pragma once

#include <map>

#include <vulkan/vulkan.hpp>

#include "backend/instance.h"
#include "common/logging.h"

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

	/**
	 * \brief Used at logical device creation time to pass the extensions feature chain to vkCreateDevice
	 * \return 
	 */
	void *get_extension_feature_chain() const;

	bool is_extension_supported(const std::string &requested_extension) const;

	vk::PhysicalDevice                            get_handle() const;
	Instance                                     &get_instance() const;
	const vk::PhysicalDeviceProperties           &get_properties() const;
	const std::vector<vk::QueueFamilyProperties> &get_queue_family_properties() const;
	vk::PhysicalDeviceFeatures                    get_features() const;
	vk::PhysicalDeviceFeatures                    get_requested_features() const;
	vk::PhysicalDeviceFeatures                   &get_mutable_requested_features();

	vk::FormatProperties get_format_properties(vk::Format format) const;

	bool has_high_priority_graphics_queue() const;

	/**
	 * \brief Get an extension features struct
	 *
	 *        Gets the actual extension features struct with the supported flags set.
	 *        The flags you're interested in can be set in a corresponding struct in the structure chain
	 *        by calling PhysicalDevice::add_extension_features()
	 */
	template <typename HPPStructureType>
	HPPStructureType get_extension_features()
	{
		// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
		if (!instance_.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
			                         " isn't enabled!");
		}

		// Get the extension feature
		return handle_.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, HPPStructureType>().template get<HPPStructureType>();
	}

	/**
	 * \brief Add an extension features struct to the structure chain used for device creation
	 *
	 *        To have the features enabled, this function must be called before the logical device
	 *        is created. To do this request sample specific features inside
	 *        VulkanSample::request_gpu_features(vkb::HPPPhysicalDevice &gpu).
	 *
	 *        If the feature extension requires you to ask for certain features to be enabled, you can
	 *        modify the struct returned by this function, it will propagate the changes to the logical
	 *        device.
	 * \returns A reference to the extension feature struct in the structure chain
	 */
	template <typename HPPStructureType>
	HPPStructureType &add_extension_features()
	{
		// We cannot request extension features if the physical device properties 2 instance extension isn't enabled
		if (!instance_.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
			                         " isn't enabled!");
		}

		// Add an (empty) extension features into the map of extension features
		auto [it, added] = extension_features_.insert({HPPStructureType::structureType, std::make_shared<HPPStructureType>()});
		if (added)
		{
			// if it was actually added, also add it to the structure chain
			if (last_requested_extension_feature_)
			{
				static_cast<HPPStructureType *>(it->second.get())->pNext = last_requested_extension_feature_;
			}
			last_requested_extension_feature_ = it->second.get();
		}

		return *static_cast<HPPStructureType *>(it->second.get());
	}

	/**
	 * \brief Request an optional features flag
	 *
	 *        Calls get_extension_features to get the support of the requested flag. If it's supported,
	 *        add_extension_features is called, otherwise a log message is generated.
	 *
	 * \returns true if the requested feature is supported, otherwise false
	 */
	template <typename Feature>
	vk::Bool32 request_optional_feature(vk::Bool32 Feature::*flag, std::string const &featureName, std::string const &flagName)
	{
		vk::Bool32 supported = get_extension_features<Feature>().*flag;
		if (supported)
		{
			add_extension_features<Feature>().*flag = true;
		}
		else
		{
			LOGI("Requested optional feature <{}::{}> is not supported", featureName, flagName);
		}
		return supported;
	}

	/**
	 * \brief Request a required features flag
	 *
	 *        Calls get_extension_features to get the support of the requested flag. If it's supported,
	 *        add_extension_features is called, otherwise a runtime_error is thrown.
	 */
	template <typename Feature>
	void request_required_feature(vk::Bool32 Feature::*flag, std::string const &featureName, std::string const &flagName)
	{
		if (get_extension_features<Feature>().*flag)
		{
			add_extension_features<Feature>().*flag = true;
		}
		else
		{
			throw std::runtime_error(std::string("Requested required feature <") + featureName + "::" + flagName + "> is not supported");
		}
	}

  private:
	Instance &instance_;

	vk::PhysicalDevice handle_{VK_NULL_HANDLE};

	vk::PhysicalDeviceFeatures         features_;
	vk::PhysicalDeviceProperties       properties_;
	vk::PhysicalDeviceMemoryProperties memory_properties_;

	// The extensions that this GPU supports
	std::vector<vk::ExtensionProperties> device_extensions_;

	// The GPU queue family properties
	std::vector<vk::QueueFamilyProperties> queue_family_properties_;

	// The features that will be requested to be enabled in the logical device
	vk::PhysicalDeviceFeatures requested_features_;

	void *last_requested_extension_feature_{nullptr};

	// Holds the extension feature structures, we use a map to retain an order of requested structures
	std::map<vk::StructureType, std::shared_ptr<void>> extension_features_;

	bool high_priority_graphics_queue_{false};
};

#define REQUEST_OPTIONAL_FEATURE(gpu, Feature, flag) gpu.request_optional_feature<Feature>(&Feature::flag, #Feature, #flag)
#define REQUEST_REQUIRED_FEATURE(gpu, Feature, flag) gpu.request_required_feature<Feature>(&Feature::flag, #Feature, #flag)

}        // namespace backend
}        // namespace xihe