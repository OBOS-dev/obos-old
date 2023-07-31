/*
	klog.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <klog.h>
#include <console.h>

namespace obos
{
	void _kpanic(CSTRING message)
	{
		// Does what we want to do, set the background colour to red, set the foreground colour to white, clear the screen, and reset the cursor position.
		InitializeConsole(ConsoleColor::WHITE, ConsoleColor::RED);
		ConsoleOutputString(message);
		asm volatile("cli;"
					 "hlt");
	}
}
