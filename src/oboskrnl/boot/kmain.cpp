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

namespace obos
{
	void _thread()
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

		thread::Thread* thread1 = new thread::Thread{};

		thread1->tid = 2;
		thread1->status = thread::THREAD_STATUS_CAN_RUN;
		thread1->priority = thread::THREAD_PRIORITY_LOW;
		thread1->exitCode = 0;
		thread1->lastError = 0;
		thread1->priorityList = thread::g_priorityLists + 1;
		thread1->threadList = thread::g_currentThread->threadList;
		thread::setupThreadContext(&thread1->context, (uintptr_t)_thread, 0, 0, false);

		thread::Thread* thread2 = new thread::Thread{};

		thread2->tid = 3;
		thread2->status = thread::THREAD_STATUS_CAN_RUN;
		thread2->priority = thread::THREAD_PRIORITY_HIGH;
		thread2->exitCode = 0;
		thread2->lastError = 0;
		thread2->priorityList = thread::g_priorityLists + 3;
		thread2->threadList = thread::g_currentThread->threadList;
		thread::setupThreadContext(&thread2->context, (uintptr_t)_thread, 0, 0, false);
		
		uintptr_t val = thread::stopTimer();
		
		thread::g_priorityLists[3].head = thread::g_priorityLists[3].tail = thread2;
		thread::g_priorityLists[3].size++;
		thread::g_priorityLists[1].head = thread::g_priorityLists[1].tail = thread1;
		thread::g_priorityLists[1].size++;

		thread1->next_list = thread2;
		thread2->prev_list = thread1;
		thread1->prev_list = thread::g_currentThread->threadList->tail;

		thread::g_currentThread->threadList->tail->next_list = thread1;
		thread::g_currentThread->threadList->tail = thread2;
		thread::g_currentThread->threadList->size += 2;

		thread::startTimer(val);
		
		thread::callScheduler();

		while (1);
	}

}