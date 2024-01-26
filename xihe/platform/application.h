#pragma once
#include <cstdint>
#include <string>

#include "platform/input_events.h"

namespace xihe
{
class Window;

class Application
{
  public:
	Application();

	virtual ~Application() = default;

	virtual bool prepare(Window *window) = 0;

	virtual void update(float delta_time) = 0;

	virtual void finish();

	const std::string &get_name() const;
	void               set_name(const std::string &name);

	virtual bool resize(const uint32_t width, const uint32_t height);

	virtual void input_event(const InputEvent &input_event);

	bool should_close() const
	{
		return requested_close_;
	}

  protected:
	Window *window_{nullptr};

  private:
	std::string name_{};
	bool        requested_close_{false};
};
}        // namespace xihe
