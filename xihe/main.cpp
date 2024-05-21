#include <Windows.h>

#include "platform/windows/windows_platform.h"
#include <fstream>
#include <iostream>

extern std::unique_ptr<xihe::Application> create_application();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
	AllocConsole();

	FILE *fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);

	if (fDummy)
	{
		fclose(fDummy);
	}

	xihe::WindowsPlatform platform{};
	xihe::Window::OptionalProperties properties{};
	properties.title = "Xi He";
	platform.set_window_properties(properties);

	const auto code = platform.initialize();
	platform.start_app("xihe", create_application);

	if (code == xihe::ExitCode::kSuccess)
	{
		platform.main_loop();
	}

	platform.terminate(code);

	FreeConsole();

	return 0;
}