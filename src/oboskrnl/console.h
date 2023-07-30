/*
	console.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include "types.h"

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
	void InitializeConsole(ConsoleColor foreground, ConsoleColor background);
	void SetCursor(bool status);
	void SetConsoleColor(ConsoleColor foreground, ConsoleColor background);
	void GetConsoleColor(ConsoleColor& foreground, ConsoleColor& background);
	void ConsoleOutputCharacter(CHAR ch);
	void ConsoleOutput(CSTRING message, SIZE_T size);
	void ConsoleOutputString(CSTRING message);
	void SetTerminalCursorPosition(DWORD x, DWORD y);
}