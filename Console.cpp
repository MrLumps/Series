#include "Console.h"


HANDLE hConIn;
HANDLE hConOut;

CONSOLE_SCREEN_BUFFER_INFO csbi;

COORD scrbufsize;

void startconsole(void) {
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);
	
	hConIn = GetStdHandle(STD_INPUT_HANDLE);
	hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

	scrbufsize.X = 160;
	scrbufsize.Y = 12000;

	//Set console framebuffer size
	SetConsoleScreenBufferSize(hConOut, scrbufsize);

	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();

}

void endconsole(void) {
	FreeConsole();
}
