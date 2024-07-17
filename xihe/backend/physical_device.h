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

	template <typename T>
	T &request_extension_features()
	{
		if (!instance_.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			throw std::runtime_error("Couldn't request feature from device as " + std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) + " isn't enabled!");
		}
	
		const vk::StructureType structure_type = T::structureType;

		const auto it = extension_features_.find(structure_type);
		if (it != extension_features_.end())
		{
			return *static_cast<T *>(it->second.get());
		}

		// Get the extension feature


		vk::PhysicalDeviceFeatures2KHR physical_device_features;
		physical_device_features = handle_.getFeatures2KHR();

		T                            extension;
		physical_device_features.pNext = &extension;
		

		// Insert the extension feature into the extension feature map so its ownership is held
		extension_features_.insert({structure_type, std::make_shared<T>(extension)});

		// Pull out the dereferenced void pointer, we can assume its type based on the template
		auto *extension_ptr = static_cast<T *>(extension_features_.find(structure_type)->second.get());


		if (last_requested_extension_feature_)
		{
			extension_ptr->pNext = last_requested_extension_feature_;
		}
		last_requested_extension_feature_ = extension_ptr;

		return *extension_ptr;
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
}        // namespace backend
}        // namespace xihe