#include "windows_platform.h"

#include "platform/glfw_window.h"

namespace xihe
{
void WindowsPlatform::create_window(const Window::Properties &properties)
{
	window_ = std::make_unique<GlfwWindow>(this, properties);
}
}
