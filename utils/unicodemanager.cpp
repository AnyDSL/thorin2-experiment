#include "utils/unicodemanager.h"
#include <iostream>
using namespace std;




#ifdef WIN32

#include <cstdio>
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#include <string>

void printUtf8(const string& input) {
	int size = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int) input.length(), NULL, 0);
	std::wstring woutput;
	woutput.resize(size);
	int result = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)input.length(), &woutput[0], size);
	std::wcout << woutput;
}

static void SetConsoleFont() {
	HANDLE StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX info;
	memset(&info, 0, sizeof(CONSOLE_FONT_INFOEX));
	info.cbSize = sizeof(CONSOLE_FONT_INFOEX);              // prevents err=87 below
	if (GetCurrentConsoleFontEx(StdOut, FALSE, &info)) {
		info.FontFamily = FF_DONTCARE;
		info.dwFontSize.X = 0;  // leave X as zero
		info.dwFontSize.Y = 16; // 14 was default
		info.FontWeight = 400;
		//wcscpy(info.FaceName, L"Lucida Console");
		wcscpy_s(info.FaceName, 9, L"Consolas");
		if (SetCurrentConsoleFontEx(StdOut, FALSE, &info)) {
		}
	}
}

void prepareUtf8Console() {
	// SetConsoleCP(65001)
	// SetConsoleOutputCP(65001);
	_setmode(_fileno(stdout), _O_U8TEXT);
	SetConsoleFont();
}





#else

void prepareUtf8Console() {}
void printUtf8(const string& input) {
	cout << input;
}

#endif
