#pragma once

#include "platform/window.h"

struct GLFWwindow;

namespace xihe
{
class Platform;

class GlfwWindow : public Window
{
  public:
	GlfwWindow(Platform *platform, const Window::Properties &properties);

	virtual ~GlfwWindow();

	VkSurfaceKHR create_surface(backend::Instance &instance) override;

	std::vector<const char *> get_required_surface_extensions() const override;

	bool should_close() override;

  private:
	GLFWwindow *handle_{nullptr};
};
}        // namespace xihe
