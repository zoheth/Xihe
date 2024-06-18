#include "platform.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "common/logging.h"
#include "common/timer.h"

#include <iostream>

namespace xihe
{

std::string Platform::working_directory_;

std::string Platform::temp_directory_;

ExitCode Platform::initialize()
{
	auto sinks = get_platform_sinks();

	auto logger = std::make_shared<spdlog::logger>("xihe", sinks.begin(), sinks.end());
#ifdef XH_DEBUG
	logger->set_level(spdlog::level::debug);
#else
	logger->set_level(spdlog::level::info);
#endif

	logger->set_pattern(LOGGER_FORMAT);
	spdlog::set_default_logger(logger);

	LOGI("Logger initialized");

	if (close_requested_)
	{
		return ExitCode::kClose;
	}

	create_window(window_properties_);
	if (!window_)
	{
		LOGE("Failed to create window");
		return ExitCode::kFatalError;
	}

	return ExitCode::kSuccess;
}

bool Platform::start_app(const std::string &app_name, const std::function<std::unique_ptr<Application>()> &create_func)
{
	application_ = create_func();

	application_->set_name(app_name);

	if (!application_)
	{
		LOGE("Failed to create a valid app.");
		return false;
	}

	if (!application_->prepare(window_.get()))
	{
		LOGE("Failed to prepare app.");
		return false;
	}

	return true;
}

ExitCode Platform::main_loop()
{
	while (!window_->should_close() && !close_requested_)
	{
#ifndef XH_DEBUG
		try
		{
#endif
			update();

			if (application_ && application_->should_close())
			{
				std::string id = application_->get_name();
				application_->finish();
			}

			window_->process_events();
#ifndef XH_DEBUG
		}

		catch (std::exception &e)
		{
			LOGE("Error Message: {}", e.what());
			LOGE("Failed when running application {}", application_->get_name());

			return ExitCode::kFatalError;
		}
#endif
	}

	return ExitCode::kSuccess;
}

void Platform::update()
{
	auto delta_time = static_cast<float>(timer_.tick<Timer::Seconds>());

	if (focused_)
	{
		application_->update(delta_time);
	}
}

void Platform::close()
{
	close_requested_ = true;
	window_->close();
}

void Platform::terminate(ExitCode code)
{
	if (application_)
	{
		application_->finish();
	}

	application_.reset();
	window_.reset();

	spdlog::drop_all();

	if (code != ExitCode::kSuccess)
	{
		std::cout << "Press return to continue";
		std::cin.get();
	}
}

void Platform::resize(uint32_t width, uint32_t height)
{
	auto extent = Window::Extent{
	    std::max<uint32_t>(width, kMinWindowWidth),
	    std::max<uint32_t>(height, kMinWindowHeight)};

	if ((window_) && (width > 0) && (height > 0))
	{
		const auto actual_extent = window_->resize(extent);

		if (application_)
		{
			application_->resize(actual_extent.width, actual_extent.height);
		}
	}
}

void Platform::input_event(const InputEvent &input_event)
{
	application_->input_event(input_event);

	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_code() == KeyCode::Back ||
		    key_event.get_code() == KeyCode::Escape)
		{
			close();
		}
	}
}

void Platform::set_focus(const bool focused)
{
	focused_ = focused;
}

void Platform::set_window_properties(const Window::OptionalProperties &properties)
{
	window_properties_.title         = properties.title.has_value() ? properties.title.value() : window_properties_.title;
	window_properties_.mode          = properties.mode.has_value() ? properties.mode.value() : window_properties_.mode;
	window_properties_.resizable     = properties.resizable.has_value() ? properties.resizable.value() : window_properties_.resizable;
	window_properties_.vsync         = properties.vsync.has_value() ? properties.vsync.value() : window_properties_.vsync;
	window_properties_.extent.width  = properties.extent.width.has_value() ? properties.extent.width.value() : window_properties_.extent.width;
	window_properties_.extent.height = properties.extent.height.has_value() ? properties.extent.height.value() : window_properties_.extent.height;
}

const std::string &Platform::get_working_directory()
{
	return working_directory_;
}

const std::string &Platform::get_temp_directory()
{
	return temp_directory_;
}

std::vector<spdlog::sink_ptr> Platform::get_platform_sinks()
{
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	return sinks;
}
}        // namespace xihe
