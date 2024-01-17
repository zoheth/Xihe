#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "backend/instance.h"

namespace xihe
{
class Window
{
  public:
	struct Extent
	{
		uint32_t width;
		uint32_t height;
	};

	enum class Mode
	{
		Headless,
		Fullscreen,
		FullscreenBorderless,
		FullscreenStretch,
		Default
	};

	enum class Vsync
	{
		OFF,
		ON,
		Default
	};

	struct Properties
	{
		std::string title     = "";
		Mode        mode      = Mode::Default;
		bool        resizable = true;
		Vsync       vsync     = Vsync::Default;
		Extent      extent    = {1280, 720};
	};

	virtual VkSurfaceKHR create_surface(backend::Instance &instance) = 0;

	virtual std::vector<const char *> get_required_surface_extensions() const = 0;
};
}        // namespace xihe
