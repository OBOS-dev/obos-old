/*
	oboskrnl/boot/kmain.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

// __ALL__ code in this file must be platform-independent.

#if defined(__x86_64__) || defined(_WIN64)
#include <limine.h>
#endif

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>
#include <error.h>

#include <new>

#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/cpu_local.h>

#include <multitasking/process/process.h>

#include <driverInterface/load.h>

#include <vfs/mount/mount.h>
#include <vfs/fileManip/fileHandle.h>

#define LITERAL(str) (char*)(str), sizeof((str))

extern obos::memory::VirtualAllocator g_liballocVirtualAllocator;

namespace obos
{
	const char* the_kernel_messages[3] = { "Hello, drivers!!! ", "This really works!!! ", "Anyway, goodbye, drivers :(\n" }; // as a test for copy relocations.
	void kmain_common(byte* initrdDriverData, size_t initrdDriverSize)
	{
		logger::log("Multitasking initialized! In \"%s\" now.\n", __func__);
		process::Process* kernelProc = new process::Process{};
		auto currentThread = thread::GetCurrentCpuLocalPtr()->currentThread;
		kernelProc->console = &g_kernelConsole;
		kernelProc->isUsermode = false;
		kernelProc->parent = nullptr;
		kernelProc->threads.head = currentThread->threadList->head;
		kernelProc->threads.tail = currentThread->threadList->tail;
		kernelProc->threads.size = currentThread->threadList->size;
		thread::Thread* thread = currentThread->next_list;
		while (thread)
		{
			thread->owner = kernelProc;
			thread->threadList = &kernelProc->threads;
		
			thread = thread->next_list;
		}
		delete currentThread->threadList;
		currentThread->threadList = &kernelProc->threads;
		currentThread->owner = kernelProc;	
#if defined(__x86_64__) || defined(_WIN64)
		kernelProc->context.cr3 = currentThread->context.cr3; // Only this time... (reference to line 7)
#endif
		// Reconstruct the liballoc's allocator with the kernel process, to avoid future problems with liballoc in user mode.
		g_liballocVirtualAllocator.~VirtualAllocator();
		new (&g_liballocVirtualAllocator) memory::VirtualAllocator{ kernelProc };
		process::g_processes.tail = process::g_processes.head = kernelProc;
		kernelProc->pid = process::g_processes.size++;
		
		logger::log("Loading the initrd driver.\n");
		SetLastError(0);
		if (!driverInterface::LoadModule(initrdDriverData, initrdDriverSize, nullptr))
			logger::panic(nullptr, "Could not load the initrd driver. GetLastError: %d\n", GetLastError());
		SetLastError(0);

		new (&vfs::g_mountPoints) utils::Vector<vfs::MountPoint*>{};

		logger::log("Mounting the initrd.\n");
		uint32_t point = 0;
		// Mount the initrd.
		if (!vfs::mount(point, 0))
			logger::panic(nullptr, "Could not mount the initrd, GetLastError: %d!\n", GetLastError());

		// Load all modules under the initrd.
		driverInterface::ScanAndLoadModules("0:/");

		thread::ExitThread(0);
	}
}