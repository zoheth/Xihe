#pragma once

#include <spdlog/common.h>

#include "common/timer.h"
#include "platform/application.h"
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

	bool start_app(const std::string &app_name, const std::function<std::unique_ptr<Application>()> &create_func);

	ExitCode main_loop();

	void update();

	virtual void terminate(ExitCode code);

	virtual void resize(uint32_t width, uint32_t height);

	virtual void input_event(const InputEvent &input_event);

	void set_focus(bool focused);

	static constexpr uint32_t kMinWindowWidth = 640;
	static constexpr uint32_t kMinWindowHeight = 480;

protected:
	virtual std::vector<spdlog::sink_ptr> get_platform_sinks();

	/**
	 * \brief Handles the creation of the window (window_)
	 * \param properties 
	 */
	virtual void create_window(const Window::Properties &properties) = 0;

	std::unique_ptr<Application> application_{nullptr};

	std::unique_ptr<Window> window_{nullptr};
	Window::Properties window_properties_{};
	bool close_requested_{false};

	bool focused_{true};

private:
	Timer timer_;
};
}        // namespace xihe