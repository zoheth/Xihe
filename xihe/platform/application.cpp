#include "application.h"

namespace xihe
{
Application::Application() :
    name_("App Name")
{}

void Application::finish()
{}

const std::string &Application::get_name() const
{
	return name_;
}

void Application::set_name(const std::string &name)
{
	name_ = name;
}

bool Application::resize(const uint32_t width, const uint32_t height)
{
	return true;
}

void Application::input_event(const InputEvent &input_event)
{}
}        // namespace xihe
