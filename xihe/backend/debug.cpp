#include "debug.h"

#include <glm/gtc/type_ptr.hpp>

#include "backend/command_buffer.h"
#include "backend/device.h"

namespace xihe::backend
{
void DebugUtilsExtDebugUtils::set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const
{
	vk::DebugUtilsObjectNameInfoEXT name_info(object_type, object_handle, name);
	device.setDebugUtilsObjectNameEXT(name_info);
}

void DebugUtilsExtDebugUtils::set_debug_tag(vk::Device device, vk::ObjectType object_type, uint64_t object_handle,
                                            uint64_t tag_name, const void *tag_data, size_t tag_data_size) const
{
	vk::DebugUtilsObjectTagInfoEXT tag_info{};
	tag_info.objectType   = object_type;
	tag_info.objectHandle = object_handle;
	tag_info.tagName      = tag_name;
	tag_info.tagSize      = tag_data_size;
	tag_info.pTag         = tag_data;

	device.setDebugUtilsObjectTagEXT(tag_info);
}

void DebugUtilsExtDebugUtils::cmd_begin_label(vk::CommandBuffer command_buffer,
                                              const char *name, glm::vec4 color) const
{
	vk::DebugUtilsLabelEXT label_info{};
	label_info.pLabelName = name;
	memcpy(label_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	command_buffer.beginDebugUtilsLabelEXT(label_info);
}

void DebugUtilsExtDebugUtils::cmd_end_label(vk::CommandBuffer command_buffer) const
{
	command_buffer.endDebugUtilsLabelEXT();
}

void DebugUtilsExtDebugUtils::cmd_insert_label(vk::CommandBuffer command_buffer,
                                               const char *name, glm::vec4 color) const
{
	vk::DebugUtilsLabelEXT label_info{};
	label_info.pLabelName = name;
	memcpy(label_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	command_buffer.insertDebugUtilsLabelEXT(label_info);
}

void DebugMarkerExtDebugUtils::set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const
{
	vk::DebugMarkerObjectNameInfoEXT name_info(vk::debugReportObjectType(object_type), object_handle, name);
	device.debugMarkerSetObjectNameEXT(name_info);
}

void DebugMarkerExtDebugUtils::set_debug_tag(vk::Device device, vk::ObjectType object_type, uint64_t object_handle,
                                             uint64_t tag_name, const void *tag_data, size_t tag_data_size) const
{
	vk::DebugMarkerObjectTagInfoEXT tag_info{};
	tag_info.objectType = vk::debugReportObjectType(object_type);
	tag_info.object     = object_handle;
	tag_info.tagName    = tag_name;
	tag_info.tagSize    = tag_data_size;
	tag_info.pTag       = tag_data;

	assert(vkDebugMarkerSetObjectTagEXT);
	device.debugMarkerSetObjectTagEXT(tag_info);
}

void DebugMarkerExtDebugUtils::cmd_begin_label(vk::CommandBuffer command_buffer,
                                               const char *name, glm::vec4 color) const
{
	vk::DebugMarkerMarkerInfoEXT marker_info{};
	marker_info.pMarkerName = name;
	memcpy(marker_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	command_buffer.debugMarkerBeginEXT(marker_info);
}

void DebugMarkerExtDebugUtils::cmd_end_label(vk::CommandBuffer command_buffer) const
{
	assert(vkCmdDebugMarkerEndEXT);
	vkCmdDebugMarkerEndEXT(command_buffer);
}

void DebugMarkerExtDebugUtils::cmd_insert_label(vk::CommandBuffer command_buffer,
                                                const char *name, glm::vec4 color) const
{
	vk::DebugMarkerMarkerInfoEXT marker_info{};
	marker_info.pMarkerName = name;
	memcpy(marker_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	command_buffer.debugMarkerInsertEXT(marker_info);
}

ScopedDebugLabel::ScopedDebugLabel(const DebugUtils &debug_utils, vk::CommandBuffer command_buffer, const char *name, glm::vec4 color) :
    debug_utils{&debug_utils},
    command_buffer{VK_NULL_HANDLE}
{
	if (name && *name != '\0')
	{
		assert(command_buffer != VK_NULL_HANDLE);
		this->command_buffer = command_buffer;

		debug_utils.cmd_begin_label(command_buffer, name, color);
	}
}

ScopedDebugLabel::ScopedDebugLabel(const CommandBuffer &command_buffer, const char *name, glm::vec4 color) :
    ScopedDebugLabel{command_buffer.get_device().get_debug_utils(), command_buffer.get_handle(), name, color}
{}

ScopedDebugLabel::~ScopedDebugLabel()
{
	if (command_buffer != VK_NULL_HANDLE)
	{
		debug_utils->cmd_end_label(command_buffer);
	}
}
}        // namespace xihe::backend
