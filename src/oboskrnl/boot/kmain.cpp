/*
	kmain.cpp

	Copyright (c) 2023 Omar Berrow
*/

/*
* TODO: Port C11 and the C++ stdlib to the operating system.
* Port this C11 implementation:		   https://github.com/managarm/mlibc (MIT License).
* Port this C++ stdlib implementation: https://sourceforge.net/projects/stlport/ (Mysterious License, but it seems like it can work with the MIT license).
*/

#include <boot/multiboot.h>

#include <types.h>
#include <console.h>
#include <klog.h>

#include <descriptors/gdt/gdt.h>
#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

#include <memory_manager/physical.h>
#include <memory_manager/paging/init.h>

#include <multitasking/multitasking.h>
#include <multitasking/thread.h>

#include <utils/memory.h>

#ifdef __INTELLISENSE__
#define __i686__ 1
#undef _MSC_VER
#endif
#include <inline-asm.h>

#if !defined(__cplusplus) || !defined(__i686__) || !defined(__ELF__) || defined(_MSC_VER)
#error You must be using a C++ i686 compiler with elf (i686-elf-g++, for example). This compiler cannot be msvc.
#endif

char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

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
		// Interrupt 32-40 (0x20-0x27)
		currentPic.remap(0x20, 4);
		currentPic.setPorts(obos::Pic::PIC2_CMD, obos::Pic::PIC2_DATA);
		// Interrupt 40-47 (0x28-0x2f)
		currentPic.remap(0x28, 2);

		for(int i = 0x20; i < 0x30; i++)
			RegisterInterruptHandler(i, [](const obos::interrupt_frame* frame) {
				if(frame->intNumber != 32)
					ConsoleOutputString("Unhandled external interrupt.");
				if (frame->intNumber == 33)
					(void)inb(0x60);
				SendEOI(frame->intNumber - 32);
			});

		asm volatile("sti");
			
		memory::InitializePhysicalMemoryManager();
		
		memory::PageDirectory g_pageDirectory{ (UINTPTR_T*)&memory::g_kernelPageDirectory };
		memory::g_pageDirectory = &g_pageDirectory;

		memory::InitializePaging();

		multitasking::InitializeMultitasking();
		// Oh no!
		kpanic("obos::kmain tried to return!");
	}
	void testThread(PVOID userData)
	{
		CSTRING str = (CSTRING)userData;
		ConsoleOutputString(str);
	}
	void kmainThr()
	{
		// Copy the page directory on the stack to the heap.
		memory::PageDirectory* pageDirectory = (memory::PageDirectory*)kcalloc(1, sizeof(memory::PageDirectory));
		utils::memcpy(pageDirectory, memory::g_pageDirectory, sizeof(memory::PageDirectory));
		memory::g_pageDirectory = pageDirectory;

		multitasking::Thread newThread1{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #1!\r\n", 0, 2 };
		multitasking::Thread newThread2{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #2!\r\n", 0, 2 };
		multitasking::Thread newThread3{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #3!\r\n", 0, 2 };
		multitasking::Thread newThread4{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #4!\r\n", 0, 2 };

		asm volatile(".byte 0xEB; .byte 0xFE;");
	}
}

extern "C"
{
	void __cxa_pure_virtual()
	{
		obos::kpanic("Attempt to call a pure virtual function.");
	}

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