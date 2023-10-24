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

#include <threadAPI/thrHandle.h>

namespace obos
{
	void _thread(uintptr_t)
	{
		logger::log("Hello from tid %d.\n", thread::g_currentThread->tid);
		uintptr_t val = thread::stopTimer();
		thread::g_currentThread->status = thread::THREAD_STATUS_DEAD;
		thread::startTimer(val);
		thread::callScheduler();
	}
	void kmain_common()
	{
		thread::g_initialized = true;
		logger::log("Multitasking initialized! In \"%s\" now.\n", __func__);
		
		thread::ThreadHandle* thread1 = new thread::ThreadHandle{};
		thread::ThreadHandle* thread2 = new thread::ThreadHandle{};
		thread::ThreadHandle* thread3 = new thread::ThreadHandle{};
		thread::ThreadHandle* thread4 = new thread::ThreadHandle{};

		thread1->CreateThread(thread::THREAD_PRIORITY_IDLE, 8192, _thread, 0);
		thread2->CreateThread(thread::THREAD_PRIORITY_NORMAL, 8192, _thread, 0);
		thread3->CreateThread(thread::THREAD_PRIORITY_LOW, 8192, _thread, 0);
		thread4->CreateThread(thread::THREAD_PRIORITY_HIGH, 8192, _thread, 0);

		thread::callScheduler();

		while (1);
	}

}