#include "window.h"

namespace xihe
{
const Window::Extent & Window::get_extent() const
{
	return properties_.extent;
}

Window::Mode Window::get_window_mode() const
{
	return properties_.mode;
}
}
