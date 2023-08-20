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
#include <memory_manager/paging/allocate.h>

#include <multitasking/multitasking.h>
#include <multitasking/threadHandle.h>

#include <driver_api/interrupts.h>
#include <driver_api/syscalls.h>

#include <process/process.h>

#include <syscalls/syscalls.h>

#include <elf/elf.h>
#include <elf/elfStructures.h>
#include <utils/memory.h>

#ifdef __INTELLISENSE__
#define __i686__ 1
#undef _MSC_VER
#undef __x86_64__
#endif

#if !defined(__cplusplus) || !defined(__i686__) || !defined(__ELF__) || defined(_MSC_VER)
#error You must be using a C++ i686 compiler with elf (i686-elf-g++, for example). This compiler cannot be msvc.
#endif

#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && ((UINTPTR_T)(val)) <= ((UINTPTR_T)(rEnd)))

extern "C" UINTPTR_T* boot_page_directory1;
extern "C" void _init();
extern "C" char _glb_text_start;
extern "C" char _glb_text_end;

namespace obos
{
	multiboot_info_t* g_multibootInfo;
	memory::PageDirectory g_pageDirectory;

	void pageFault(const interrupt_frame* frame)
	{
		const char* action = utils::testBitInBitfield(frame->errorCode, 1) ? "write" : "read";
		const char* privilegeLevel = utils::testBitInBitfield(frame->errorCode, 2) ? "user" : "kernel";
		const char* isPresent = utils::testBitInBitfield(frame->errorCode, 0) ? "present" : "non-present";
		UINTPTR_T location = (UINTPTR_T)memory::GetPageFaultAddress();
		extern UINT32_T* s_backbuffer;
		if (multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->isUserMode)
			multitasking::g_currentThread->owner->TerminateProcess(0xFFFFFFF1);
		if (!inRange(location, s_backbuffer, s_backbuffer + 1024 * 768))
			kpanic((PVOID)frame->ebp, (PVOID)frame->eip,
			kpanic_format(
				"Page fault in %s-mode at %p (tid %d, pid %d) while trying to %s a %s page.\r\nThe address of that page is %p (Page directory index %d, page table index %d).\r\nDumping registers: \r\n"
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
				privilegeLevel, frame->eip, multitasking::GetCurrentThreadTid(), multitasking::g_currentThread->owner->pid, action, isPresent, location,
				memory::PageDirectory::addressToIndex(location), memory::PageDirectory::addressToPageTableIndex(location),
				frame->edi, frame->esi, frame->ebp, frame->esp, frame->ebx,
				frame->edx, frame->ecx, frame->eax, frame->eip, frame->eflags,
				frame->ss, frame->ds, frame->cs);
		asm volatile("cli; hlt");
	}
	void defaultExceptionHandler(const interrupt_frame* frame)
	{
		if (frame->errorCode == 1 && utils::testBitInBitfield(frame->eflags, 8))
		{
			utils::clearBitInBitfield(const_cast<interrupt_frame*>(frame)->eflags, 8);
			return;
		}
		if (multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->isUserMode)
			multitasking::g_currentThread->owner->TerminateProcess(~frame->intNumber);
		kpanic((PVOID)frame->ebp, (PVOID)frame->eip,
			kpanic_format(
				"Unhandled exception %d at %p (tid %d, pid %d). Error code: %d. Dumping registers: \r\n"
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
				frame->intNumber,
				frame->eip,
				multitasking::GetCurrentThreadTid(), multitasking::g_currentThread->owner->pid,
				frame->errorCode,
				frame->edi, frame->esi, frame->ebp, frame->esp, frame->ebx,
				frame->edx, frame->ecx, frame->eax, frame->eip, frame->eflags,
				frame->ss, frame->ds, frame->cs);
	}

	// Initial boot sequence (Initializes the gdt, the idt, paging, the console, the physical memory manager, the pic, and software multitasking.
	void kmain(multiboot_info_t* header, DWORD magic)
	{
		obos::g_multibootInfo = header;

		if (magic != MULTIBOOT_BOOTLOADER_MAGIC || header->mods_count != NUM_MODULES)
			return;
		
		EnterKernelSection();

		memory::g_pageDirectory = &g_pageDirectory;

		memory::InitializePaging();

		RegisterInterruptHandler(14, pageFault);

		obos::InitializeGdt();
		obos::InitializeIdt();

		memory::InitializePhysicalMemoryManager();

		if ((g_multibootInfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) != MULTIBOOT_INFO_FRAMEBUFFER_INFO)
			kpanic(nullptr, getEIP(), kpanic_format("No framebuffer info from the bootloader.\r\n"));
		if (g_multibootInfo->framebuffer_height != 768 || g_multibootInfo->framebuffer_width != 1024)
			kpanic(nullptr, getEIP(), kpanic_format("The framebuffer set up by the bootloader is not 1024x768. Instead, it is %dx%d\r\n"),
				g_multibootInfo->framebuffer_width, g_multibootInfo->framebuffer_height);

		for (UINTPTR_T physAddress = g_multibootInfo->framebuffer_addr, virtAddress = 0xFFCFF000;
			virtAddress < 0xFFFFF000;
			virtAddress += 4096, physAddress += 4096)
			memory::kmap_physical((PVOID)virtAddress, 1, memory::VirtualAllocFlags::WRITE_ENABLED, (PVOID)physAddress);

		// Initialize the console.
		InitializeConsole(0xFFFFFFFF, 0x00000000);

		obos::Pic currentPic{ obos::Pic::PIC1_CMD, obos::Pic::PIC1_DATA };
		// Interrupt 32-40 (0x20-0x27)
		currentPic.remap(0x20, 4);
		currentPic.setPorts(obos::Pic::PIC2_CMD, obos::Pic::PIC2_DATA);
		// Interrupt 40-47 (0x28-0x2f)
		currentPic.remap(0x28, 2);

		for(BYTE i = 0x20; i < 0x30; i++)
			RegisterInterruptHandler(i, [](const obos::interrupt_frame* frame) {
				SendEOI(frame->intNumber - 32);
				});
		for (BYTE i = 0; i < 32; i++)
			RegisterInterruptHandler(i == 14 ? 0 : i, defaultExceptionHandler);

		LeaveKernelSection();

		multitasking::InitializeMultitasking();
		// Oh no!
		kpanic(nullptr, getEIP(), kpanic_format("obos::kmain tried to return!"));
	}
	process::Process* g_kernelProcess = nullptr;
	void kmainThr()
	{ 
		multitasking::g_initialized = true;

		_init();
		g_kernelProcess = new process::Process{};

		g_kernelProcess->pageDirectory = memory::g_pageDirectory;
		g_kernelProcess->pid = process::g_nextProcessId++;
		g_kernelProcess->threads = list_new();
		g_kernelProcess->abstractHandles = list_new();
		g_kernelProcess->children = list_new();

		list_rpush(g_kernelProcess->threads, list_node_new(multitasking::g_currentThread));

		multitasking::g_currentThread->owner = g_kernelProcess;

		driverAPI::ResetSyscallHandlers();

		driverAPI::RegisterSyscalls();

		process::RegisterSyscalls();
		
		{
			PCHAR ptr = (PCHAR)memory::VirtualAlloc((PVOID)0x400000, 1, memory::VirtualAllocFlags::GLOBAL | memory::VirtualAllocFlags::WRITE_ENABLED);

			memory::tlbFlush((UINT32_T)ptr);

			for (PCHAR it = &_glb_text_start; it != &_glb_text_end; it++, ptr++)
				*ptr = *it;
			memory::MemoryProtect(ptr, 1, memory::VirtualAllocFlags::GLOBAL);
		}

		multitasking::ThreadHandle mainThread;

		process::Process* initrdDriver = new process::Process{};
		auto ret = 
			initrdDriver->CreateProcess(
				(PBYTE)((multiboot_module_t*)g_multibootInfo->mods_addr)[2].mod_start, 
				((multiboot_module_t*)g_multibootInfo->mods_addr)[2].mod_end - ((multiboot_module_t*)g_multibootInfo->mods_addr)[2].mod_start,
				(PVOID)&mainThread, true);
		if (ret)
			kpanic(nullptr, getEIP(), kpanic_format("CreateProcess failed with %d."), ret);
		mainThread.WaitForThreadStatusChange((DWORD)multitasking::Thread::status_t::RUNNING | (DWORD)multitasking::Thread::status_t::BLOCKED);
		mainThread.closeHandle();

		char(*existsCallback)(CSTRING filename, SIZE_T* size) = (char(*)(CSTRING filename, SIZE_T* size))driverAPI::g_registeredDrivers[1]->existsCallback;
		void(*readCallback)(CSTRING filename, STRING output, SIZE_T size) = (void(*)(CSTRING filename, STRING output, SIZE_T size))driverAPI::g_registeredDrivers[1]->readCallback;

		SIZE_T filesize = 0;
		initrdDriver->pageDirectory->switchToThis();
		char existsData = existsCallback("ahci", &filesize);
		multitasking::g_currentThread->owner->pageDirectory->switchToThis();
		if (!existsData)
			kpanic(nullptr, getEIP(), kpanic_format("/obos/initrd/ahci doesn't exist."));
		PBYTE filedata = new BYTE[filesize];
		initrdDriver->pageDirectory->switchToThis();
		readCallback("ahci", (STRING)filedata, filesize);
		multitasking::g_currentThread->owner->pageDirectory->switchToThis();

		process::Process* ahciDriver = new process::Process{};
		ahciDriver->CreateProcess(filedata, filesize, (PVOID)&mainThread, true);
		delete[] filedata;
		if (ret)
			kpanic(nullptr, getEIP(), kpanic_format("CreateProcess failed with %d."), ret);
		mainThread.WaitForThreadStatusChange((DWORD)multitasking::Thread::status_t::RUNNING | (DWORD)multitasking::Thread::status_t::BLOCKED);
		mainThread.closeHandle();

		filesize = 0;
		initrdDriver->pageDirectory->switchToThis();
		existsData = existsCallback("ps2Keyboard", &filesize);
		multitasking::g_currentThread->owner->pageDirectory->switchToThis();
		if (!existsData)
			kpanic(nullptr, getEIP(), kpanic_format("/obos/initrd/ps2Keyboard doesn't exist."));
		filedata = new BYTE[filesize];
		initrdDriver->pageDirectory->switchToThis();
		readCallback("ps2Keyboard", (STRING)filedata, filesize);
		multitasking::g_currentThread->owner->pageDirectory->switchToThis();

		process::Process* keyboardDriver = new process::Process{};
		keyboardDriver->CreateProcess(filedata, filesize, (PVOID)&mainThread, true);
		delete[] filedata;
		if (ret)
			kpanic(nullptr, getEIP(), kpanic_format("CreateProcess failed with %d."), ret);
		mainThread.WaitForThreadStatusChange((DWORD)multitasking::Thread::status_t::RUNNING | (DWORD)multitasking::Thread::status_t::BLOCKED);
		mainThread.closeHandle();

		char* ascii_art = (STRING)((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_start;
		
		SetConsoleColor(0x003399FF, 0x00000000);
		ConsoleOutput(ascii_art, ((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_end - ((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_start);
		SetConsoleColor(0xFFFFFFFF, 0x00000000);

		// Uncomment this line to kpanic.
		//*((PBYTE)0x486594834) = 'L';

		asm volatile (".byte 0xeb, 0xfe;");
	}

}

extern "C"
{
	void __cxa_pure_virtual()
	{
		obos::kpanic(nullptr, getEIP(), kpanic_format("Attempt to call a pure virtual function."));
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