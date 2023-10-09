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

#include <driver_api/driver_interface.h>
#include <driver_api/interrupts.h>
#include <driver_api/syscalls.h>
#include <driver_api/load/module.h>

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
extern "C" void _start(PVOID);
extern "C" char _glb_text_start;
extern "C" char _glb_text_end;
extern "C" void idleTask(PVOID);
extern bool inKernelSection;

#ifdef __i686__
#error i686 (x86) will no longer be supported :(
#endif

namespace obos
{
	namespace memory
	{
		extern UINTPTR_T* kmap_pageTable(PVOID physicalAddress);
		extern UINTPTR_T* g_zeroPage;
	}
#if defined(__x86_64__)
	obos::multiboot_info* g_multibootInfo;
	static obos::multiboot_info s_multibootInfo;
	static obos::multiboot_module_t s_modules[NUM_MODULES];
#elif defined(__i686__)
	multiboot_info_t* g_multibootInfo;
	memory::PageDirectory g_pageDirectory;
#endif
	extern char kernelStart;
	
	extern void pageFault(const interrupt_frame* frame);
	extern void debugExceptionHandler(const interrupt_frame* frame);
	extern void defaultExceptionHandler(const interrupt_frame* frame);

	static void cursorUpdate(PVOID)
	{
		extern DWORD s_terminalColumn;
		extern DWORD s_terminalRow;
		static bool s_isCursorOn = false;
		static SIZE_T s_cursorRow = 0;
		static SIZE_T s_cursorColumn = 0;
		static bool s_cursorInitialized;
		while(1)
		{
			multitasking::Sleep(CURSOR_SLEEP_TIME_MS);
			if (s_cursorRow != s_terminalRow)
			{
				if (s_cursorInitialized)
					ConsoleOutputCharacter(' ', s_cursorColumn, s_cursorRow);
				s_cursorRow = s_terminalRow;
			}
			s_cursorInitialized = true;
			if (s_cursorColumn != s_terminalColumn)
			{
				if (s_cursorColumn > s_terminalColumn)
					ConsoleOutputCharacter(' ', s_cursorColumn + 1, s_cursorRow);
				s_cursorColumn = s_terminalColumn;
			}
			if (s_isCursorOn && s_cursorRow == s_terminalRow)
			{
				ConsoleOutputCharacter('_', s_cursorColumn++, s_cursorRow);
				s_cursorColumn--;
			}
			else
				ConsoleOutputCharacter(' ', s_cursorColumn, s_cursorRow);
			s_isCursorOn = !s_isCursorOn;
		}
	}

	// Initial boot sequence. Initializes all platform-specific things, and sets up multitasking, and jumps to kmainThr.
	void kmain(::multiboot_info_t* header, DWORD magic)
	{
#if defined(__i686__)
		obos::g_multibootInfo = reinterpret_cast<::multiboot_info_t*>(reinterpret_cast<UINTPTR_T>(header) + reinterpret_cast<UINTPTR_T>(&kernelStart));
		if (magic != MULTIBOOT_BOOTLOADER_MAGIC || g_multibootInfo->mods_count != NUM_MODULES)
			return;
#elif defined(__x86_64__)
		g_multibootInfo = &s_multibootInfo;
		if (magic != MULTIBOOT_BOOTLOADER_MAGIC || header->mods_count != NUM_MODULES)
			return;
#endif


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
			::multiboot_module_t* modules = (multiboot_module_t*)g_multibootInfo->mods_addr;
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
#if defined(__x86_64__)
		RegisterInterruptHandler(1, debugExceptionHandler);
		for (BYTE i = 0; i < 32; i++)
			RegisterInterruptHandler((i == 14 || i == 1) ? 0 : i, defaultExceptionHandler);
#else
		for (BYTE i = 0; i < 32; i++)
			RegisterInterruptHandler(i == 14 ? 0 : i, defaultExceptionHandler);
#endif

		memory::InitializePaging();

		memory::InitializePhysicalMemoryManager();

#ifdef __x86_64__
		memory::g_zeroPage = reinterpret_cast<UINTPTR_T*>(memory::kalloc_physicalPage());
		utils::dwMemset((DWORD*)memory::kmap_pageTable(memory::g_zeroPage), 0, 4096 / 4);
#endif

		extern UINT32_T* s_framebuffer;

		if ((g_multibootInfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) != MULTIBOOT_INFO_FRAMEBUFFER_INFO)
			kpanic(nullptr, getEIP(), kpanic_format("No framebuffer info from the bootloader.\r\n"));
#ifndef __x86_64__
		if (g_multibootInfo->framebuffer_height != 768 || g_multibootInfo->framebuffer_width != 1024)
			kpanic(nullptr, getEIP(), kpanic_format("The framebuffer set up by the bootloader is not 1024x768. Instead, it is %dx%d\r\n"),
				g_multibootInfo->framebuffer_width, g_multibootInfo->framebuffer_height);
#endif

		UINTPTR_T framebuffer_limit = 0;

#if defined(__x86_64__)
		s_framebuffer = (UINT32_T*)0xFFFFFFFF80600000;
		framebuffer_limit = 0xFFFFFFFF80600000 + (static_cast<UINTPTR_T>(g_multibootInfo->framebuffer_height) * g_multibootInfo->framebuffer_width * 4);
#elif defined(__i686__)
		s_framebuffer = (UINT32_T*)0xFFCFF000;
		framebuffer_limit = 0xFFFFF000;
#endif

		for (UINTPTR_T physicalAddress = g_multibootInfo->framebuffer_addr, virtAddress = (UINTPTR_T)s_framebuffer;
			virtAddress < framebuffer_limit;
			virtAddress += 4096, physicalAddress += 4096)
			memory::kmap_physical((PVOID)virtAddress, memory::VirtualAllocFlags::WRITE_ENABLED, (PVOID)physicalAddress);

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
		LeaveKernelSection();

		multitasking::InitializeMultitasking();
		// Oh no!
		kpanic(nullptr, nullptr, kpanic_format("obos::kmain tried to return!"));
	}

	process::Process* g_kernelProcess = nullptr;
	void kmainThr()
	{ 
		multitasking::g_initialized = true;

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
#if defined(__i686__)
			process::ProcEntryPointBase = 0x400000;
#elif defined(__x86_64__)
			process::ProcEntryPointBase = 0x1000000;
#endif
			PCHAR ptr = (PCHAR)memory::VirtualAlloc((PVOID)process::ProcEntryPointBase, 1, memory::VirtualAllocFlags::GLOBAL | memory::VirtualAllocFlags::WRITE_ENABLED);

			memory::tlbFlush((UINTPTR_T)ptr);

			for (PCHAR it = &_glb_text_start; it != &_glb_text_end; it++, ptr++)
				*ptr = *it;
			memory::MemoryProtect(ptr, 1, memory::VirtualAllocFlags::GLOBAL);
		}

		// Initialize the console.
		InitializeConsole(0xFFFFFFFF, 0x00000000);
		
		multitasking::ThreadHandle mainThread;

		driverAPI::driverIdentification* initrdDriver = 
			driverAPI::LoadModule((PBYTE)((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_start,
				((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_end - ((multiboot_module_t*)g_multibootInfo->mods_addr)[1].mod_start, &mainThread);

		obos::driverAPI::driver_commands request = obos::driverAPI::driver_commands::OBOS_SERVICE_READ_FILE;
		driverAPI::DriverClientConnectionHandle driverCon;
		driverCon.OpenConnection(initrdDriver->driverId);
		driverCon.SendData((PBYTE)&request, sizeof(request));
		BYTE filepath[] = { "testProgram" };
		SIZE_T filepathSize = sizeof(filepath);
		//SIZE_T nul = 0;
		driverCon.SendData((PBYTE)&filepathSize, sizeof(filepathSize));
		driverCon.SendData(filepath, filepathSize);
		SIZE_T filesize = 0;
		driverCon.RecvData((PBYTE)&filesize, sizeof(filesize));
		PBYTE contents = new BYTE[filesize + 1];
		driverCon.RecvData(contents, filesize);

		process::Process* proc = new process::Process;
		proc->CreateProcess(contents, filesize, nullptr);
		
		delete[] contents;

		multitasking::ThreadHandle handle;
		handle.OpenThread(multitasking::g_currentThread);
		handle.SetThreadPriority(multitasking::Thread::priority_t::HIGH);
		handle.closeHandle();
			
		cursorUpdate(nullptr);

		RestartComputer();
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