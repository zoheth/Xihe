#pragma once
#include <spdlog/common.h>

#include "platform/window.h"
#include "platform/input_events.h"

namespace xihe
{
enum class ExitCode
{
	kSuccess = 0, /* App executed as expected */
	kHelp,        /* App should show help */
	kClose,       /* App has been requested to close at initialization */
	kFatalError   /* App encountered an unexpected error */
};
class Platform
{
public:
	virtual ExitCode initialize();

	virtual void resize(uint32_t width, uint32_t height);

	virtual void input_event(const InputEvent &input_event);

protected:
	virtual std::vector<spdlog::sink_ptr> get_platform_sinks();

	/**
	 * \brief Handles the creation of the window (window_)
	 * \param properties 
	 */
	virtual void create_window(const Window::Properties &properties) = 0;

	std::unique_ptr<Window> window_{nullptr};
	Window::Properties window_properties_{};
	bool close_requested_{false};
};
}        // namespace xihe