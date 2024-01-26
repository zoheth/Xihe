#include <Windows.h>
#undef max
#undef min

#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "platform/windows/windows_platform.h"

extern std::unique_ptr<xihe::Application> create_application();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
	xihe::WindowsPlatform platform{};

	auto code = platform.initialize();
	platform.start_app("xihe", create_application);

	if (code == xihe::ExitCode::kSuccess)
	{
		platform.main_loop();
	}

	platform.terminate(code);

	return 0;
}