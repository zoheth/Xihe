#include  "debug.h"

namespace xihe::backend
{
void DebugUtilsExtDebugUtils::set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const
{
	vk::DebugUtilsObjectNameInfoEXT name_info(object_type, object_handle, name);
	device.setDebugUtilsObjectNameEXT(name_info);
}

void DebugMarkerExtDebugUtils::set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const
{
	vk::DebugMarkerObjectNameInfoEXT name_info(vk::debugReportObjectType(object_type), object_handle, name);
	device.debugMarkerSetObjectNameEXT(name_info);
}
}
