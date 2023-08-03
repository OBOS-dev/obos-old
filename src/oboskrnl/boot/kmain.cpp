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
#include <inline-asm.h>
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
#undef __x86_64__
#endif

#if !defined(__cplusplus) || !defined(__i686__) || !defined(__ELF__) || defined(_MSC_VER)
#error You must be using a C++ i686 compiler with elf (i686-elf-g++, for example). This compiler cannot be msvc.
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
		// Interrupt 32-40 (0x20-0x27)
		currentPic.remap(0x20, 4);
		currentPic.setPorts(obos::Pic::PIC2_CMD, obos::Pic::PIC2_DATA);
		// Interrupt 40-47 (0x28-0x2f)
		currentPic.remap(0x28, 2);

		for(int i = 0x20; i < 0x30; i++)
			RegisterInterruptHandler(i, [](const obos::interrupt_frame* frame) {
				if(frame->intNumber != 32)
					printf("\r\nUnhandled external interrupt. IRQ Number: %d.\r\n", frame->intNumber - 32);
				if (frame->intNumber == 33)
					(void)inb(0x60);
				SendEOI(frame->intNumber - 32);
			});
		for (int i = 0; i < 32; i++)
		{
			RegisterInterruptHandler(i, [](const interrupt_frame* frame) {
					kpanic(kpanic_format(
						"Unhandled exception %d at %p. Error code: %d. Dumping registers: \r\n"
						"\tEDI: %p\r\n"
						"\tESI: %p\r\n"
						"\tEBP: %p\r\n"
						"\tEBP: %p\r\n"
						"\tEBX: %p\r\n"
						"\tEDX: %p\r\n"
						"\tECX: %p\r\n"
						"\tEAX: %p\r\n"
						"\tEIP: %p\r\n"
						"\tEFLAGS: %p\r\n"
						"\tSS: %p\r\n"
						"\tDS: %p\r\n"
						"\tCS: %p\r\n"),
						frame->intNumber,
						frame->eip,
						frame->errorCode,
						frame->edi, frame->esi, frame->ebp, frame->esp, frame->ebx,
						frame->edx, frame->ecx, frame->eax, frame->eip, frame->eflags,
						frame->eflags,
						frame->ss, frame->ds, frame->cs);
				});
		}

		RegisterInterruptHandler(14, [](const interrupt_frame* frame) {
			const char* action = utils::testBitInBitfield(frame->errorCode, 1) ? "write" : "read";
			const char* privilegeLevel = utils::testBitInBitfield(frame->errorCode, 2) ? "user" : "kernel";
			const char* isPresent = utils::testBitInBitfield(frame->errorCode, 0) ? "present" : "non-present";
			UINTPTR_T location = (UINTPTR_T)memory::GetPageFaultAddress();
			kpanic(kpanic_format(
				"Page fault in %s-mode at %p while trying to %s a %s page. The address of that page is %p (Page directory index %d, page table index %d).\r\n Dumping registers: \r\n"
				"\tEDI: %p\r\n"
				"\tESI: %p\r\n"
				"\tEBP: %p\r\n"
				"\tESP: %p\r\n"
				"\tEBX: %p\r\n"
				"\tEDX: %p\r\n"
				"\tECX: %p\r\n"
				"\tEAX: %p\r\n"
				"\tEIP: %p\r\n"
				"\tEFLAGS: %p\r\n"
				"\tSS: %p\r\n"
				"\tDS: %p\r\n"
				"\tCS: %p\r\n"),
				privilegeLevel, frame->eip, action, isPresent, location,
				memory::PageDirectory::addressToIndex(location), memory::PageDirectory::addressToPageTableIndex(location), 
				frame->edi, frame->esi, frame->ebp, frame->esp, frame->ebx,
				frame->edx, frame->ecx, frame->eax, frame->eip, frame->eflags,
				frame->eflags,
				frame->ss, frame->ds, frame->cs);
			});

		asm volatile("sti");
			
		memory::InitializePhysicalMemoryManager();
		
		memory::PageDirectory g_pageDirectory{ (UINTPTR_T*)&memory::g_kernelPageDirectory };
		memory::g_pageDirectory = &g_pageDirectory;

		memory::InitializePaging();

		multitasking::InitializeMultitasking();
		// Oh no!
		kpanic(kpanic_format("obos::kmain tried to return!"));
	}
	void testThread(PVOID userData)
	{
		CSTRING str = (CSTRING)userData;
		ConsoleOutputString(str);
		multitasking::g_currentThread->status = (UINT32_T)multitasking::Thread::status_t::DEAD;
		_int(0x30);
	}
	void kmainThr()
	{
		memory::g_pageDirectory = new memory::PageDirectory{ memory::g_kernelPageDirectory };

		multitasking::Thread* newThread1 = new multitasking::Thread{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #1!\r\n", 0, 2 };
		multitasking::Thread* newThread2 = new multitasking::Thread{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #2!\r\n", 0, 2 };
		multitasking::Thread* newThread3 = new multitasking::Thread{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #3!\r\n", 0, 2 };
		multitasking::Thread* newThread4 = new multitasking::Thread{ multitasking::Thread::priority_t::NORMAL, testThread, (PVOID)"Hello from test thread #4!\r\n", 0, 2 };

		asm volatile(".byte 0xEB; .byte 0xFE;");

		delete newThread1;
		delete newThread2;
		delete newThread3;
		delete newThread4;
	}

}

extern "C"
{
	void __cxa_pure_virtual()
	{
		obos::kpanic(kpanic_format("Attempt to call a pure virtual function."));
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