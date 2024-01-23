#include "platform.h"

#include "common/logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace xihe
{
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

std::vector<spdlog::sink_ptr> Platform::get_platform_sinks()
{
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	return sinks;
}
}        // namespace xihe
