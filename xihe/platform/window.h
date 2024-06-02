#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

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

	struct OptionalExtent
	{
		std::optional<uint32_t> width;
		std::optional<uint32_t> height;
	};
	enum class Mode
	{
		kHeadless,
		kFullscreen,
		kFullscreenBorderless,
		kFullscreenStretch,
		kDefault
	};

	enum class Vsync
	{
		OFF,
		ON,
		kDefault
	};

	struct OptionalProperties
	{
		std::optional<std::string> title;
		std::optional<Mode>        mode;
		std::optional<bool>        resizable;
		std::optional<Vsync>       vsync;
		OptionalExtent        extent;
	};

	struct Properties
	{
		std::string title;
		Mode        mode      = Mode::kDefault;
		bool        resizable = true;
		Vsync       vsync     = Vsync::kDefault;
		Extent      extent    = {1280, 720};
	};

	Window(const Properties &properties);

	virtual VkSurfaceKHR create_surface(backend::Instance &instance) = 0;

	virtual bool should_close() = 0;

	virtual void close() = 0;

	virtual void process_events();

	Extent resize(const Extent &extent);

	virtual std::vector<const char *> get_required_surface_extensions() const = 0;

	virtual bool get_display_present_info(vk::DisplayPresentInfoKHR *info,
		uint32_t src_width, uint32_t src_height) const;

	const Extent     &get_extent() const;
	Mode              get_window_mode() const;
	const Properties &get_properties() const
	{
		return properties_;
	}

  protected:
	Properties properties_;
};
}        // namespace xihe
