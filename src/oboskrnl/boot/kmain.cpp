/*
	oboskrnl/boot/kmain.cpp

	Copyright (c) 2023 Omar Berrow
*/

// __ALL__ code in this file must be platform-independent.

#include <int.h>
#include <klog.h>

#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/arch.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <multitasking/process/process.h>

namespace obos
{
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
		thread::g_currentThread->owner = kernelProc;

		logger::log("Done booting!\n");
		thread::ExitThread(0);
	}

}