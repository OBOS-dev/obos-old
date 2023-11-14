/*
	oboskrnl/multitasking/scheduler.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <multitasking/thread.h>

namespace obos
{
	namespace thread
	{
		extern Thread::ThreadList g_priorityLists[4];
		extern bool g_initialized;
		extern uint64_t g_schedulerFrequency;
		extern uint32_t g_nextTid;
		extern uint64_t g_timerTicks;
		void InitializeScheduler();
	}
}