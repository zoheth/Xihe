#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class DebugUtils
{
  public:
	virtual ~DebugUtils() = default;

	/**
	 * @brief Sets the debug name for a Vulkan object.
	 */
	virtual void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const = 0;
};

/**
 * @brief HPPDebugUtils implemented on top of VK_EXT_debug_utils.
 */
class DebugUtilsExtDebugUtils final : public backend::DebugUtils
{
  public:
	~DebugUtilsExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;
};

class DebugMarkerExtDebugUtils final : public backend::DebugUtils
{
  public:
	~DebugMarkerExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;
};

class DummyDebugUtils final : public backend::DebugUtils
{
  public:
	~DummyDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override
	{}
};

}        // namespace xihe::backend
