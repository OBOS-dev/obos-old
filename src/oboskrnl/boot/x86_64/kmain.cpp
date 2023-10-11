/*
	oboskrnl/boot/x86_64/main.cpp

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __x86_64__
#error Invalid architecture.
#endif

#include <int.h>
#include <console.h>
#include <klog.h>

#include <x86_64-utils/asm.h>

#include <arch/interrupt.h>

#include <memory_manipulation.h>

#include <limine.h>

bool strcmp(const char* str1, const char* str2)
{
	if (obos::utils::strlen(str1) != obos::utils::strlen(str2))
		return false;
	for (size_t i = 0; str1[i]; i++)
		if (str1[i] != str2[i])
			return false;
	return true;
}

namespace obos
{
	void InitializeGdt();
	void InitializeIdt();
	Console g_kernelConsole{};
	static volatile limine_framebuffer_request framebuffer_request = {
		.id = LIMINE_FRAMEBUFFER_REQUEST,
		.revision = 0
	};
	static volatile limine_module_request module_request = {
		.id = LIMINE_MODULE_REQUEST,
		.revision = 0,
	};
	void EarlyKPanic()
	{
		outb(0x64, 0xFE);
		cli();
		while (1)
			hlt();
	}
	// Responsible for: Setting up the CPU-Specific features. Setting up IRQs. Initialising the memory manager, and the console.
	void arch_kmain()
	{
		con_framebuffer framebuffer;
		void* font = nullptr;
		if (!framebuffer_request.response->framebuffers)
			EarlyKPanic();
		
		framebuffer.addr = (uint32_t*)framebuffer_request.response->framebuffers[0]->address;
		framebuffer.width = framebuffer_request.response->framebuffers[0]->width;
		framebuffer.height = framebuffer_request.response->framebuffers[0]->height;
		for (size_t i = 0; i < module_request.response->module_count; i++)
		{
			if (strcmp(module_request.response->modules[i]->path, "/obos/font.bin"))
			{
				font = (uint8_t*)module_request.response->modules[i]->address;
				break;
			}
		}
		g_kernelConsole.Initialize(font, framebuffer);
		g_kernelConsole.SetColour(0xffffffff, 0);
		InitializeGdt();
		InitializeIdt();
		int counter = 0;
		logger::log("This is a test log message. Counter: %d\n", counter++);
		logger::warning("This is a test warning. Counter: %d\n", counter++);
		logger::error("This is a test error. Counter: %d\n", counter++);
		logger::panic("This is a test panic. Counter: %d\n", counter++);
		EarlyKPanic();
	}
}