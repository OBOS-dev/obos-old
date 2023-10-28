/*
	oboskrnl/boot/kmain.cpp

	Copyright (c) 2023 Omar Berrow
*/

// __ALL__ code in this file must be platform-independent.

#include <limine.h>

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/arch.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <multitasking/process/process.h>
#include <multitasking/process/x86_64/loader/elf.h>

#include <driverInterface/load.h>
#include <driverInterface/call.h>
#include <driverInterface/interface.h>

namespace obos
{
	extern volatile limine_module_request module_request;
	void _thread(uintptr_t)
	{
		logger::log("Hello from tid %d.\n", thread::g_currentThread->tid);
		thread::ExitThread(0);
	}
	void kmain_common()
	{
		thread::g_initialized = true;
		logger::log("Multitasking initialized! In \"%s\" now.\n", __func__);
		
		process::Process* kernelProc = new process::Process;
		kernelProc->pid = 0;
		kernelProc->isUsermode = false;
		kernelProc->parent = nullptr;
#ifdef __x86_64__
		kernelProc->context.cr3 = thread::g_currentThread->context.cr3; // Only this time... (reference to line 7)
#endif
		process::g_processes.tail = process::g_processes.head = kernelProc;
		process::g_processes.size++;
		thread::g_currentThread->owner = kernelProc;

		byte* procExecutable = nullptr;
		size_t moduleIndex = 0;

		for (; moduleIndex < module_request.response->module_count; moduleIndex++)
		{
			if (utils::strcmp(module_request.response->modules[moduleIndex]->path, "/obos/testDriver"))
			{
				procExecutable = (byte*)module_request.response->modules[moduleIndex]->address;
				break;
			}
		}

		if (!procExecutable)
			logger::panic("Couldn't find the test driver.\n");
		
		driverInterface::RegisterSyscall(0, (uintptr_t)(void(*)(const char**))[](const char** str) {
			g_kernelConsole.ConsoleOutput(*str); 
			});
		driverInterface::RegisterSyscall(1, (uintptr_t)(void(*)(const uint32_t*))[](const uint32_t* exitCode) { 
			thread::ExitThread(*exitCode); 
			});
		driverInterface::RegisterSyscall(2, (uintptr_t)driverInterface::SyscallAllocDriverServer);
		driverInterface::RegisterSyscall(3, (uintptr_t)driverInterface::SyscallFreeDriverServer);

		uint32_t pid = driverInterface::LoadModule(procExecutable, module_request.response->modules[moduleIndex]->size, nullptr);
		
		driverInterface::DriverClient client;
		client.OpenConnection(pid, 1000);

		uint32_t i = 0;
		client.RecvData((byte*)&i, sizeof(i));
		logger::printf("Received %d from the driver.\n", i);
		i = 420;
		logger::printf("Sending %d.\n", i);
		client.SendData((byte*)&i, sizeof(i));
		client.CloseConnection();

		thread::ExitThread(0);
	}

}