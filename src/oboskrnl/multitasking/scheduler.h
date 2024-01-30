/*
	oboskrnl/multitasking/scheduler.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <export.h>

#include <multitasking/thread.h>

namespace obos
{
	namespace thread
	{
		extern Thread::ThreadList g_priorityLists[4];
		extern bool g_initialized;
		extern OBOS_EXPORT uint64_t g_schedulerFrequency;
		extern OBOS_EXPORT uint64_t g_timerTicks;
		extern uint32_t g_nextTid;
		void InitializeScheduler();
	}
}