/*
	kmain.cpp

	Copyright (c) 2023 Omar Berrow
*/

/*
* TODO: Port C11 and the C++ stdlib to the operating system.
* Port this C11 implementation:		   https://github.com/managarm/mlibc (MIT License).
* Port this C++ stdlib implementation: https://github.com/LiveMirror/stlport (Mysterious License, but it seems like it can work with the MIT license).
*/

#include <boot/multiboot.h>

#include <types.h>
#include <console.h>
#include <klog.h>
#include <inline-asm.h>

#include <descriptors/gdt/gdt.h>
#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

#include <memory_manager/physical.h>
#include <memory_manager/paging/init.h>

#if !defined(__cplusplus) && !defined(__i686__) && !defined(__ELF__)
#error You must be using a C++ i686 compiler with elf. (i686-elf-g++, for example)
#endif

namespace obos
{
	multiboot_info_t* g_multibootInfo = nullptr;
	extern void* stack_top;

	// I just love name mangling.
	// _ZN4obos5kmainEP14multiboot_infoj
	void kmain(multiboot_info_t* header, DWORD magic)
	{
		if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
			return;
		obos::g_multibootInfo = header;

		// Initialize the console.
		obos::InitializeConsole(obos::ConsoleColor::LIGHT_GREY, obos::ConsoleColor::BLACK);

		obos::InitializeGdt();
		obos::InitializeIdt();

		obos::Pic currentPic{ obos::Pic::PIC1_CMD, obos::Pic::PIC1_DATA };
		// Interrupt 30-37 (0x20-0x27)
		currentPic.remap(0x20, 4);
		currentPic.setPorts(obos::Pic::PIC2_CMD, obos::Pic::PIC2_DATA);
		// Interrupt 40-47 (0x28-0x2f)
		currentPic.remap(0x28, 2);

		for(int i = 0x20; i < 0x30; i++)
			obos::RegisterInterruptHandler(i, [](const obos::interrupt_frame* frame) {
			if(frame->intNumber != 32)
				obos::ConsoleOutputString("Unhandled external interrupt.");
			int irqNumber = frame->intNumber - 32;
			obos::Pic masterPic{ obos::Pic::PIC1_CMD, obos::Pic::PIC1_DATA };
			if(irqNumber < 7 && masterPic.issuedInterrupt(irqNumber))
				masterPic.sendEOI();
			if(irqNumber > 7)
			{
				masterPic.setPorts(obos::Pic::PIC2_CMD, obos::Pic::PIC2_DATA);
				// masterPic is now slavePic.
				if(masterPic.issuedInterrupt(irqNumber))
					masterPic.sendEOI();
			}
			});

		sti();
			
		obos::memory::InitializePhysicalMemoryManager();
		obos::memory::InitializePaging();

		obos::ConsoleOutputString("Hello, world!\r\n");
	}
}

extern "C" void __cxa_pure_virtual()
{
	obos::kpanic("Attempt to call a pure virtual function.");
}

extern "C"
{
	typedef unsigned uarch_t;
	
	struct atexitFuncEntry_t {
		void (*destructorFunc) (void*);
		void* objPtr;
		void* dsoHandle;
	
	};
	
	#define ATEXIT_FUNC_MAX 256
	
	atexitFuncEntry_t __atexitFuncs[ATEXIT_FUNC_MAX];
	uarch_t __atexitFuncCount = 0;
	
	extern void* __dso_handle;
	
	int __cxa_atexit(void (*f)(void*), void* objptr, void* dso) {
		if (__atexitFuncCount >= ATEXIT_FUNC_MAX) {
			return -1;
		}
		__atexitFuncs[__atexitFuncCount].destructorFunc = f;
		__atexitFuncs[__atexitFuncCount].objPtr = objptr;
		__atexitFuncs[__atexitFuncCount].dsoHandle = dso;
		__atexitFuncCount++;
		return 0;
	}
	
	void __cxa_finalize(void* f) {
		signed i = __atexitFuncCount;
		if (!f) {
			while (i--) {
				if (__atexitFuncs[i].destructorFunc) {
					(*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
				}
			}
			return;
		}
	
		for (; i >= 0; i--) {
			if (__atexitFuncs[i].destructorFunc == f) {
				(*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
				__atexitFuncs[i].destructorFunc = 0;
			}
		}
	}
}