#include "window.h"

namespace xihe
{
Window::Window(const Properties &properties) :
    properties_(properties)
{}

void Window::process_events()
{}

Window::Extent Window::resize(const Extent &extent)
{
	if (properties_.resizable)
	{
		properties_.extent.width  = extent.width;
		properties_.extent.height = extent.height;
	}

	return properties_.extent;
}

bool Window::get_display_present_info(vk::DisplayPresentInfoKHR *info, uint32_t src_width, uint32_t src_height) const
{
	// Default is to not use the extra present info
	return false;
}

const Window::Extent &Window::get_extent() const
{
	return properties_.extent;
}

Window::Mode Window::get_window_mode() const
{
	return properties_.mode;
}
}        // namespace xihe
