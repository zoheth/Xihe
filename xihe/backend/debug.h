#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "common/glm_common.h"

namespace xihe::backend
{
class DebugUtils
{
  public:
	virtual ~DebugUtils() = default;

	virtual void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const = 0;

	virtual void set_debug_tag(vk::Device device, vk::ObjectType object_type, uint64_t object_handle,
	                           uint64_t tag_name, const void *tag_data, size_t tag_data_size) const = 0;

	virtual void cmd_begin_label(vk::CommandBuffer command_buffer,
	                             const char *name, glm::vec4 color = {}) const = 0;

	/**
	 * @brief Inserts a command to end the current debug label/marker scope.
	 */
	virtual void cmd_end_label(vk::CommandBuffer command_buffer) const = 0;

	/**
	 * @brief Inserts a (non-scoped) debug label/marker in the command buffer.
	 */
	virtual void cmd_insert_label(vk::CommandBuffer command_buffer,
	                              const char *name, glm::vec4 color = {}) const = 0;
};

/**
 * @brief HPPDebugUtils implemented on top of VK_EXT_debug_utils.
 */
class DebugUtilsExtDebugUtils final : public backend::DebugUtils
{
  public:
	~DebugUtilsExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;

	void set_debug_tag(vk::Device device, vk::ObjectType object_type, uint64_t object_handle,
	                   uint64_t tag_name, const void *tag_data, size_t tag_data_size) const override;

	void cmd_begin_label(vk::CommandBuffer command_buffer,
	                     const char *name, glm::vec4 color) const override;

	void cmd_end_label(vk::CommandBuffer command_buffer) const override;

	void cmd_insert_label(vk::CommandBuffer command_buffer,
	                      const char *name, glm::vec4 color) const override;
};

class DebugMarkerExtDebugUtils final : public backend::DebugUtils
{
  public:
	~DebugMarkerExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;

	void set_debug_tag(vk::Device device, vk::ObjectType object_type, uint64_t object_handle,
	                   uint64_t tag_name, const void *tag_data, size_t tag_data_size) const override;

	void cmd_begin_label(vk::CommandBuffer command_buffer,
	                     const char *name, glm::vec4 color) const override;

	void cmd_end_label(vk::CommandBuffer command_buffer) const override;

	void cmd_insert_label(vk::CommandBuffer command_buffer,
	                      const char *name, glm::vec4 color) const override;
};

class DummyDebugUtils final : public backend::DebugUtils
{
  public:
	~DummyDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override
	{}

	inline void set_debug_tag(vk::Device, vk::ObjectType, uint64_t,
	                          uint64_t, const void *, size_t) const override
	{}

	inline void cmd_begin_label(vk::CommandBuffer,
	                            const char *, glm::vec4) const override
	{}

	inline void cmd_end_label(vk::CommandBuffer) const override
	{}

	inline void cmd_insert_label(vk::CommandBuffer,
	                             const char *, glm::vec4) const override
	{}
};

class CommandBuffer;

/**
 * @brief A RAII debug label.
 *        If any of EXT_debug_utils or EXT_debug_marker is available, this:
 *        - Begins a debug label / marker on construction
 *        - Ends it on destruction
 */
class ScopedDebugLabel final
{
  public:
	ScopedDebugLabel(const DebugUtils &debug_utils, vk::CommandBuffer command_buffer,
	                 const char *name, glm::vec4 color = {});

	ScopedDebugLabel(const CommandBuffer &command_buffer,
	                 const char *name, glm::vec4 color = {});

	~ScopedDebugLabel();

  private:
	const DebugUtils *debug_utils;
	vk::CommandBuffer   command_buffer;
};

}        // namespace xihe::backend
