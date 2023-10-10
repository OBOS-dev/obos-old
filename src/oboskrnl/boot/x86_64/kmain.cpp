/*
	oboskrnl/boot/x86_64/main.cpp

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __x86_64__
#error Invalid architeture.
#endif

#include <console.h>

#include <x86_64-utils/asm.h>

namespace obos
{
	void EarlyKPanic()
	{
		outb(0x64, 0xFE);
		cli();
		while (1)
			hlt();
	}
	void kmain()
	{
		ConsoleOutput("Hello, world!");
		cli();
		while (1)
			hlt();
	}
}