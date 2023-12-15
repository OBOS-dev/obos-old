/*
	oboskrnl/boot/kmain.cpp

	Copyright (c) 2023 Omar Berrow
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
#include <driverInterface/call.h>

#include <vfs/mount/mount.h>
#include <vfs/fileManip/fileHandle.h>

#define LITERAL(str) (char*)(str), sizeof((str))

namespace obos
{
	void kmain_common()
	{
		logger::log("Multitasking initialized! In \"%s\" now.\n", __func__);

		process::Process* kernelProc = new process::Process;
		kernelProc->pid = 0;
		kernelProc->console = &g_kernelConsole;
		kernelProc->isUsermode = false;
		kernelProc->parent = nullptr;
#ifdef __x86_64__
		kernelProc->context.cr3 = thread::GetCurrentCpuLocalPtr()->currentThread->context.cr3; // Only this time... (reference to line 7)
#endif
		process::g_processes.tail = process::g_processes.head = kernelProc;
		process::g_processes.size++;
		thread::GetCurrentCpuLocalPtr()->currentThread->owner = kernelProc;
		thread::GetCurrentCpuLocalPtr()->currentThread->next_list->owner = kernelProc;

		byte* procExecutable = nullptr;
		size_t moduleIndex = 0;
#if defined(__x86_64__) || defined(_WIN64)
		extern volatile limine_module_request module_request;
		for (; moduleIndex < module_request.response->module_count; moduleIndex++)
		{
			if (utils::strcmp(module_request.response->modules[moduleIndex]->path, "/obos/initrdDriver"))
			{
				procExecutable = (byte*)module_request.response->modules[moduleIndex]->address;
				break;
			}
		}

		if (!procExecutable)
			logger::panic(nullptr, "Couldn't find the initrd driver.\n");
#endif

		logger::log("Initializing driver syscalls.\n");
		driverInterface::RegisterSyscall(0, (uintptr_t)driverInterface::SyscallVPrintf);
		driverInterface::RegisterSyscall(1, (uintptr_t)(void(*)(const uint32_t*))[](const uint32_t* exitCode) {
			thread::ExitThread(*exitCode);
			});
		driverInterface::RegisterSyscall(2, (uintptr_t)driverInterface::SyscallAllocDriverServer);
		driverInterface::RegisterSyscall(3, (uintptr_t)driverInterface::SyscallFreeDriverServer);
		driverInterface::RegisterSyscall(4, (uintptr_t)driverInterface::SyscallMalloc);
		driverInterface::RegisterSyscall(5, (uintptr_t)driverInterface::SyscallFree);
		driverInterface::RegisterSyscall(6, (uintptr_t)driverInterface::SyscallMapPhysToVirt);
		driverInterface::RegisterSyscall(7, (uintptr_t)driverInterface::SyscallGetInitrdLocation);

		logger::log("Loading the initrd driver.\n");
		SetLastError(0);
		if (!driverInterface::LoadModule(procExecutable, module_request.response->modules[moduleIndex]->size, nullptr))
			logger::panic(nullptr, "Could not load the initrd driver. GetLastError: %d", GetLastError());
		SetLastError(0);

		new (&vfs::g_mountPoints) Vector<vfs::MountPoint*>{};

		logger::log("Mounting the initrd.\n");
		uint32_t point = 0;
		// Mount the initrd.
		if (!vfs::mount(point, 0))
			logger::panic(nullptr, "Could not mount the initrd, GetLastError: %d!\n", GetLastError());

		vfs::FileHandle handle;
		handle.Open("0:/test.txt");
		char* data = new char[handle.GetFileSize() + 1];
		handle.Read(data, handle.GetFileSize());
		logger::printf("0:/test.txt:\n%s\n", data);
		delete[] data;
		//handle.Close();

		thread::ExitThread(0);
	}
}