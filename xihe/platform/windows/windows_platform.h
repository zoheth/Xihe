#pragma once

#include "platform/platform.h"

namespace xihe
{
	class WindowsPlatform : public Platform
	{
	protected:
		void create_window(const Window::Properties &properties) override;
	};
}