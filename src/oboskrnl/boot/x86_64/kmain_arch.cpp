/*
	oboskrnl/boot/x86_64/kmain_arch.cpp

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __x86_64__
#error Invalid architecture.
#endif

#include <new>

#include <int.h>
#include <console.h>
#include <klog.h>

#include <x86_64-utils/asm.h>

#include <arch/interrupt.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <arch/x86_64/irq/irq.h>
#include <arch/x86_64/irq/timer.h>

#include <arch/x86_64/syscall/register.h>

#include <memory_manipulation.h>

#include <limine.h>

#include <multitasking/scheduler.h>

#define CPUID_FSGSBASE (1)
#define CPUID_SMEP (1<<7)
#define CPUID_SMAP (1<<20)
#define CR4_FSGSBASE ((uintptr_t)1<<16)
#define CR4_SMEP ((uintptr_t)1<<20)
#define CR4_SMAP ((uintptr_t)1<<21)

extern "C" void fpuInit();
extern "C" void initialize_syscall_instruction();

namespace obos
{	
	extern void InitializeGdt();
	extern void InitializeIdt();
	extern void RegisterExceptionHandlers();
	extern void initSSE();
	Console g_kernelConsole{};
	volatile limine_framebuffer_request framebuffer_request = {
		.id = LIMINE_FRAMEBUFFER_REQUEST,
		.revision = 0
	};
	volatile limine_module_request module_request = {
		.id = LIMINE_MODULE_REQUEST,
		.revision = 0,
	};
	static volatile limine_stack_size_request stack_size_request = {
		.id = LIMINE_STACK_SIZE_REQUEST,
		.revision = 0,
		.stack_size = (4 * 4096)
	};
	void EarlyKPanic();

	void enableSMEP_SMAP();

	// Responsible for: Setting up the CPU-Specific features. Setting up IRQs. Initialising the memory manager, and the console. This also must enable the scheduler.
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
			if (utils::strcmp(module_request.response->modules[i]->path, "/obos/font.bin"))
			{
				font = (uint8_t*)module_request.response->modules[i]->address;
				break;
			}
		}
		new (&g_kernelConsole) Console { font, framebuffer, false };
		g_kernelConsole.SetColour(0xffffffff, 0);
		logger::info("%s: Initializing the boot GDT.\n", __func__);
		InitializeGdt();
		logger::info("%s: Initializing IDT.\n", __func__);
		InitializeIdt();
		logger::info("%s: Registering exception handlers.\n", __func__);
		RegisterExceptionHandlers();
		logger::info("%s: Initializing IRQs.\n", __func__);
		InitializeIrq(true);
		logger::info("%s: Initializing the physical memory manager.\n", __func__);
		memory::InitializePhysicalMemoryManager();
		logger::info("%s: Initializing the virtual memory manager.\n", __func__);
		memory::InitializeVirtualMemoryManager();
		logger::info("%s: Initializing the 0x87 fpu.\n", __func__);
		fpuInit();
		logger::info("%s: Initializing SSE.\n", __func__);
		initSSE();
		uint32_t unused = 0, rbx = 0;
		__cpuid__(0x7, 0, &unused, &rbx, &unused, &unused);
		OBOS_ASSERTP(rbx & CPUID_FSGSBASE, "The current CPU doesn't support wr*sbase/rd*sbase!\n");
		logger::info("%s: Enabling SMEP/SMAP if supported.\n", __func__);
		enableSMEP_SMAP();
		logger::info("%s: Enabling \"WRFSBASE/WRGSBASE\" instructions.\n", __func__);
		setCR4(getCR4() | CR4_FSGSBASE);
		logger::info("%s: Initializing \"syscall/sysret\" instructions.\n", __func__);
		initialize_syscall_instruction();
		logger::info("%s: Registering all syscalls.\n", __func__);
		syscalls::RegisterSyscalls();
		logger::info("%s: Initializing the scheduler.\n", __func__);
		thread::InitializeScheduler();
		logger::panic(nullptr, "Failed to initialize the scheduler.");
	}
	void enableSMEP_SMAP()
	{
		uint32_t unused = 0, ebx = 0;
		__cpuid__(0x7, 0, &unused, &ebx, &unused, &unused);
		if (ebx & CPUID_SMEP)
			setCR4(getCR4() | CR4_SMEP);
		if (ebx & CPUID_SMAP)
			setCR4(getCR4() | CR4_SMAP);
	}
	void EarlyKPanic()
	{
		outb(0x64, 0xFE);
		cli();
		while (1)
			hlt();
	}
	namespace utils
	{
		bool strcmp(const char* str1, const char* str2)
		{
			if (obos::utils::strlen(str1) != obos::utils::strlen(str2))
				return false;
			for (size_t i = 0; str1[i]; i++)
				if (str1[i] != str2[i])
					return false;
			return true;
		}
	}
}