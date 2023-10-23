/*
	oboskrnl/boot/x86_64/kmain_arch.cpp

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

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <arch/x86_64/irq/irq.h>
#include <arch/x86_64/irq/timer.h>

#include <memory_manipulation.h>

#include <limine.h>

#include <multitasking/scheduler.h>

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
	void RegisterExceptionHandlers();
	Console g_kernelConsole{};
	static volatile limine_framebuffer_request framebuffer_request = {
		.id = LIMINE_FRAMEBUFFER_REQUEST,
		.revision = 0
	};
	static volatile limine_module_request module_request = {
		.id = LIMINE_MODULE_REQUEST,
		.revision = 0,
	};
	static volatile limine_stack_size_request stack_size_request = {
		.id = LIMINE_STACK_SIZE_REQUEST,
		.revision = 0,
		.stack_size = (4 * 4096)
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
		framebuffer.pitch = framebuffer_request.response->framebuffers[0]->pitch;
		if (framebuffer_request.response->framebuffers[0]->bpp != 32)
			EarlyKPanic();
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
		logger::info("%s: Initializing GDT.\n", __func__);
		InitializeGdt();
		logger::info("%s: Initializing IDT.\n", __func__);
		InitializeIdt();
		logger::info("%s: Registering exception handlers.\n", __func__);
		RegisterExceptionHandlers();
		logger::info("%s: Initializing IRQs.\n", __func__);
		InitializeIrq();
		logger::info("%s: Initializing the physical memory manager.\n", __func__);
		memory::InitializePhysicalMemoryManager();
		logger::info("%s: Initializing the virtual memory manager.\n", __func__);
		memory::InitializeVirtualMemoryManager();
		logger::info("%s: Initializing the scheduler.\n", __func__);
		thread::InitializeScheduler();
		logger::panic("Failed to initialize the scheduler.");
	}
}