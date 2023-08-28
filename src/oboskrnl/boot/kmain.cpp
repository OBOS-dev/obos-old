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

#include <boot/boot.h>

#ifdef __INTELLISENSE__
#undef _MSC_VER
#endif

#if !defined(__cplusplus) || !defined(__ELF__)
#error You must be using a C++ compiler with elf.
#endif

#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && ((UINTPTR_T)(val)) < ((UINTPTR_T)(rEnd)))

extern "C" UINTPTR_T* boot_page_directory1;
extern "C" void _init();
extern "C" char _glb_text_start;
extern "C" char _glb_text_end;

namespace obos
{
#if defined(__x86_64__)
	obos::multiboot_info* g_multibootInfo;
	static obos::multiboot_info s_multibootInfo;
	static obos::multiboot_module_t s_modules;
#elif defined(__i686__)
	multiboot_info_t* g_multibootInfo;
	memory::PageDirectory g_pageDirectory;
#endif
	extern char kernelStart;
	
	extern void pageFault(const interrupt_frame* frame);
	extern void defaultExceptionHandler(const interrupt_frame* frame);

	// Initial boot sequence. Initializes all platform-specific things, and sets up multitasking, and jumps to kmainThr.
	void kmain(::multiboot_info_t* header, DWORD magic)
	{
#if defined(__i686__)
		obos::g_multibootInfo = reinterpret_cast<::multiboot_info_t*>(reinterpret_cast<UINTPTR_T>(header) + reinterpret_cast<UINTPTR_T>(&kernelStart));
#elif defined(__x86_64__)
		g_multibootInfo = &s_multibootInfo;
#endif

		if (magic != MULTIBOOT_BOOTLOADER_MAGIC || header->mods_count != NUM_MODULES)
			return;

#if defined(__x86_64__)
		g_multibootInfo->flags = header->flags;
		g_multibootInfo->mem_upper = header->mem_upper;
		g_multibootInfo->mem_lower = header->mem_lower;
		g_multibootInfo->boot_device = header->boot_device;
		g_multibootInfo->cmdline = header->cmdline;
		g_multibootInfo->mods_addr = reinterpret_cast<UINTPTR_T>(&s_modules) - reinterpret_cast<UINTPTR_T>(&kernelStart);
		g_multibootInfo->mods_count = header->mods_count;
		utils::memcpy(&g_multibootInfo->u, &header->u, sizeof(header->u));
		g_multibootInfo->mmap_addr = header->mmap_addr;
		g_multibootInfo->mmap_length = header->mmap_length;
		g_multibootInfo->drives_length = header->drives_length;
		g_multibootInfo->drives_addr = header->drives_addr;
		g_multibootInfo->config_table = header->config_table;
		g_multibootInfo->boot_loader_name = header->boot_loader_name;
		g_multibootInfo->apm_table = header->apm_table;
		g_multibootInfo->vbe_control_info = header->vbe_control_info;
		g_multibootInfo->vbe_mode_info = header->vbe_mode_info;
		g_multibootInfo->vbe_mode = header->vbe_mode;
		g_multibootInfo->vbe_interface_seg = header->vbe_interface_seg;
		g_multibootInfo->vbe_interface_off = header->vbe_interface_off;
		g_multibootInfo->vbe_interface_len = header->vbe_interface_len;
		g_multibootInfo->framebuffer_addr = header->framebuffer_addr;
		g_multibootInfo->framebuffer_pitch = header->framebuffer_pitch;
		g_multibootInfo->framebuffer_width = header->framebuffer_width;
		g_multibootInfo->framebuffer_height = header->framebuffer_height;
		g_multibootInfo->framebuffer_bpp = header->framebuffer_bpp;
		g_multibootInfo->framebuffer_type = header->framebuffer_type;
		utils::memcpy(&g_multibootInfo->framebuffer_palette_addr, &header->framebuffer_palette_addr, 6);

		{
			obos::multiboot_module_t* modules = (obos::multiboot_module_t*)g_multibootInfo->mods_addr;
			::multiboot_module_t* modules32 = (::multiboot_module_t*)g_multibootInfo->mods_addr;
			for (SIZE_T i = 0; i < g_multibootInfo->mods_count; i++)
			{
				modules[i].cmdline = modules32[i].cmdline + reinterpret_cast<UINTPTR_T>(&kernelStart);
				modules[i].mod_start += modules32[i].mod_start + reinterpret_cast<UINTPTR_T>(&kernelStart);
				modules[i].mod_end += modules32[i].mod_end + reinterpret_cast<UINTPTR_T>(&kernelStart);
			}
		}
#elif defined(__i686__)
		memory::g_pageDirectory = &g_pageDirectory;
#endif

		g_multibootInfo->apm_table += reinterpret_cast<UINTPTR_T>(&kernelStart);
		g_multibootInfo->boot_loader_name += reinterpret_cast<UINTPTR_T>(&kernelStart);
		g_multibootInfo->cmdline += reinterpret_cast<UINTPTR_T>(&kernelStart);
		g_multibootInfo->config_table += reinterpret_cast<UINTPTR_T>(&kernelStart);
		g_multibootInfo->drives_addr += reinterpret_cast<UINTPTR_T>(&kernelStart);
		g_multibootInfo->mmap_addr += reinterpret_cast<UINTPTR_T>(&kernelStart);
		g_multibootInfo->mods_addr += reinterpret_cast<UINTPTR_T>(&kernelStart);
#if defined(__x86_64__)
		{
			obos::multiboot_module_t* modules = (obos::multiboot_module_t*)g_multibootInfo->mods_addr;
			::multiboot_module_t* real_modules = (::multiboot_module_t*)((UINTPTR_T)header->mods_addr);
			for (SIZE_T i = 0; i < g_multibootInfo->mods_count; i++)
			{
				modules[i].cmdline = real_modules[i].cmdline;
				modules[i].mod_start = real_modules[i].mod_start;
				modules[i].mod_end = real_modules[i].mod_end;
				modules[i].cmdline += reinterpret_cast<UINTPTR_T>(&kernelStart);
				modules[i].mod_start += reinterpret_cast<UINTPTR_T>(&kernelStart);
				modules[i].mod_end += reinterpret_cast<UINTPTR_T>(&kernelStart);
			}
		}
#elif defined(__i686__)
		{
			::multiboot_module_t* modules = (obos::multiboot_module_t*)g_multibootInfo->mods_addr;
			for(SIZE_T i = 0; i < g_multibootInfo->mods_count; i++)
			{
				modules[i].cmdline += reinterpret_cast<UINTPTR_T>(&kernelStart);
				modules[i].mod_start += reinterpret_cast<UINTPTR_T>(&kernelStart);
				modules[i].mod_end += reinterpret_cast<UINTPTR_T>(&kernelStart);
			}
		}
#endif
		
		EnterKernelSection();

		obos::InitializeGdt();
		obos::InitializeIdt();

		RegisterInterruptHandler(14, pageFault);
		
		memory::InitializePhysicalMemoryManager();

		memory::InitializePaging();

		if ((g_multibootInfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) != MULTIBOOT_INFO_FRAMEBUFFER_INFO)
			kpanic(nullptr, getEIP(), kpanic_format("No framebuffer info from the bootloader.\r\n"));
		if (g_multibootInfo->framebuffer_height != 768 || g_multibootInfo->framebuffer_width != 1024)
			kpanic(nullptr, getEIP(), kpanic_format("The framebuffer set up by the bootloader is not 1024x768. Instead, it is %dx%d\r\n"),
				g_multibootInfo->framebuffer_width, g_multibootInfo->framebuffer_height);

		for (UINTPTR_T physAddress = g_multibootInfo->framebuffer_addr, virtAddress = 0xFFCFF000;
			virtAddress < 0xFFFFF000;
			virtAddress += 4096, physAddress += 4096)
			memory::kmap_physical((PVOID)virtAddress, memory::VirtualAllocFlags::WRITE_ENABLED, (PVOID)physAddress);

		// Initialize the console.
		InitializeConsole(0xFFFFFFFF, 0x00000000);

		ConsoleOutputString("No way!\r\nWe're on x86-64.");

		while (1);

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

		//_init();
		g_kernelProcess = new process::Process{};

#if defined(__i686__)
		g_kernelProcess->pageDirectory = memory::g_pageDirectory;
#elif defined(__x86_64__)
		g_kernelProcess->level4PageMap = memory::g_level4PageMap;
#endif
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

			memory::tlbFlush((UINTPTR_T)ptr);

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
		initrdDriver->doContextSwitch();
		char existsData = existsCallback("nvme", &filesize);
		multitasking::g_currentThread->owner->doContextSwitch();
		if (!existsData)
			kpanic(nullptr, getEIP(), kpanic_format("/obos/initrd/nvme doesn't exist."));
		PBYTE filedata = new BYTE[filesize];
		initrdDriver->doContextSwitch();
		readCallback("nvme", (STRING)filedata, filesize);
		multitasking::g_currentThread->owner->doContextSwitch();

		process::Process* nvmeDriver = new process::Process{};
		nvmeDriver->CreateProcess(filedata, filesize, (PVOID)&mainThread, true);
		delete[] filedata;
		if (ret)
			kpanic(nullptr, getEIP(), kpanic_format("CreateProcess failed with %d."), ret);
		//mainThread.WaitForThreadStatusChange(0);
		// We don't need the handle anymore.
		mainThread.closeHandle();

		filesize = 0;
		initrdDriver->doContextSwitch();
		existsData = existsCallback("ps2Keyboard", &filesize);
		multitasking::g_currentThread->owner->doContextSwitch();
		if (!existsData)
			kpanic(nullptr, getEIP(), kpanic_format("/obos/initrd/ps2Keyboard doesn't exist."));
		filedata = new BYTE[filesize];
		initrdDriver->doContextSwitch();
		readCallback("ps2Keyboard", (STRING)filedata, filesize);
		multitasking::g_currentThread->owner->doContextSwitch();

		process::Process* keyboardDriver = new process::Process{};
		keyboardDriver->CreateProcess(filedata, filesize, (PVOID)&mainThread, true);
		delete[] filedata;
		if (ret)
			kpanic(nullptr, getEIP(), kpanic_format("CreateProcess failed with %d."), ret);
		//mainThread.WaitForThreadStatusChange(0);
		// We don't need the handle anymore.
		mainThread.closeHandle();

		char* ascii_art = (STRING)((multiboot_mod_list64*)g_multibootInfo->mods_addr)[1].mod_start;
		
		SetConsoleColor(0x003399FF, 0x00000000);
		ConsoleOutputString("\n");
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
	
	void* __dso_handle = (void*)0x27F187081FADCF;
	
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