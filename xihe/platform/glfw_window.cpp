#include "glfw_window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace xihe
{
VkSurfaceKHR GlfwWindow::create_surface(backend::Instance &instance)
{
	VkInstance vk_instance = instance.get_handle();
	if (vk_instance == VK_NULL_HANDLE || !handle_)
	{
		return VK_NULL_HANDLE;
	}

	VkSurfaceKHR surface;

	VkResult result = glfwCreateWindowSurface(vk_instance, handle_, nullptr, &surface);
	if (result != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}
	return surface;
}

std::vector<const char *> GlfwWindow::get_required_surface_extensions() const
{
	uint32_t     glfw_extension_count{0};
	const char **names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	return {names, names + glfw_extension_count};
}
}        // namespace xihe
