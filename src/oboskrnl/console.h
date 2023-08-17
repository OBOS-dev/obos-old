/*
	console.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

namespace obos
{
	enum class ConsoleColor
	{
		BLACK,
		BLUE,
		GREEN,
		CYAN,
		RED,
		MAGENTA,
		BROWN,
		LIGHT_GREY,
		DARK_GREY,
		LIGHT_BLUE,
		LIGHT_GREEN,
		LIGHT_CYAN,
		LIGHT_RED,
		LIGHT_MAGENTA,
		LIGHT_BROWN,
		YELLOW = LIGHT_BROWN,
		WHITE,
	};
	void InitializeConsole(UINT32_T foreground, UINT32_T background);
	void SetConsoleColor(UINT32_T foreground, UINT32_T background);
	void GetConsoleColor(UINT32_T& foreground, UINT32_T& background);
	void ConsoleOutputCharacter(CHAR ch, bool swapBuffers = true);
	void ConsoleOutputCharacter(CHAR ch, DWORD x, DWORD y, bool swapBuffers = true);
	void ConsoleOutput(CSTRING message, SIZE_T size, bool swapBuffers = true);
	void ConsoleOutputString(CSTRING message, bool swapBuffers = true);
	void ConsoleFillLine(char ch = '-', bool _swapBuffers = true);
	void SetTerminalCursorPosition(DWORD x, DWORD y);
	void GetTerminalCursorPosition(DWORD& x, DWORD& y);

	void swapBuffers();
}