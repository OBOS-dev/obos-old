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

#include <elf/elf.h>

#include <driver_api/interrupts.h>
#include <driver_api/syscalls.h>

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
		if (!inRange(location, s_backbuffer, s_backbuffer + 1024 * 768))
			kpanic((PVOID)frame->ebp, kpanic_format(
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
				frame->ss, frame->ds, frame->cs);
		asm volatile("cli; hlt");
	}
	void defaultExceptionHandler(const interrupt_frame* frame)
	{
		kpanic((PVOID)frame->ebp, kpanic_format(
			"Unhandled exception %d at %p. Error code: %d. Dumping registers: \r\n"
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
			kpanic(nullptr, kpanic_format("No framebuffer info from the bootloader.\r\n"));
		if (g_multibootInfo->framebuffer_height != 768 || g_multibootInfo->framebuffer_width != 1024)
			kpanic(nullptr, kpanic_format("The framebuffer set up by the bootloader is not 1024x768. Instead, it is %dx%d\r\n"),
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
		//kpanic(kpanic_format("obos::kmain tried to return!"));
	}
	void(*readFromKeyboard)(STRING outputBuffer, SIZE_T sizeBuffer) = nullptr;
	void kmainThr()
	{ 
		multitasking::g_initialized = true;

		_init();

		UINTPTR_T entry = 0, base = 0;

		driverAPI::ResetSyscallHandlers();

		driverAPI::RegisterSyscalls();
		
		DWORD err = elfLoader::LoadElfFile((PBYTE)((multiboot_module_t*)g_multibootInfo->mods_addr)[2].mod_start, 
			((multiboot_module_t*)g_multibootInfo->mods_addr)[2].mod_end - ((multiboot_module_t*)g_multibootInfo->mods_addr)[2].mod_start,
			entry,base);
		if (err)
			kpanic(nullptr, kpanic_format("Elf loader failed with code %d."), err);
		((DWORD(*)())entry)();

		char(*existsCallback)(CSTRING filename, SIZE_T* size) = (char(*)(CSTRING filename, SIZE_T* size))driverAPI::g_registeredDrivers[1]->existsCallback;
		void(*readCallback)(CSTRING filename, STRING output, SIZE_T size) = (void(*)(CSTRING filename, STRING output, SIZE_T size))driverAPI::g_registeredDrivers[1]->readCallback;
		
		SIZE_T filesize = 0;
		char existsData = existsCallback("ahci", &filesize);
		if (!existsData)
			kpanic(nullptr, kpanic_format("/obos/initrd/ahci doesn't exist."));
		char* filedata = new char[filesize];
		readCallback("ahci", filedata, filesize);

		err = elfLoader::LoadElfFile((PBYTE)filedata,
			filesize,
			entry, base);
		if (err)
			kpanic(nullptr, kpanic_format("Elf loader failed with code %d."), err);
		delete[] filedata;

		((DWORD(*)())entry)();

		/*char* ascii_art = (STRING)((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_start;
		
		SetConsoleColor(0x003399FF, 0x00000000);
		ConsoleOutput(ascii_art, ((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_end - ((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_start);
		SetConsoleColor(0xFFFFFFFF, 0x00000000);*/

		asm volatile (".byte 0xeb, 0xfe;");
	}

}

extern "C"
{
	void __cxa_pure_virtual()
	{
		obos::kpanic(nullptr, kpanic_format("Attempt to call a pure virtual function."));
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