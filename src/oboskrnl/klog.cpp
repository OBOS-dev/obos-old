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
		SetConsoleColor(ConsoleColor::WHITE, ConsoleColor::RED);
		ConsoleOutputString(message);
		while (1);
	}
}
