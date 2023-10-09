/*
	multitasking.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <utils/list.h>

#include <multitasking/thread.h>

namespace obos
{
	namespace multitasking
	{
		extern list_t* g_threads;
		// g_threadPriorityList[0] is for idle threads.
		// g_threadPriorityList[1] is for low-priority threads.
		// g_threadPriorityList[2] is for normal-priority threads.
		// g_threadPriorityList[3] is for high-priority threads.
		extern list_t* g_threadPriorityList[4];
		extern Thread* g_currentThread;
		extern bool g_initialized;
		extern SIZE_T g_timerTicks;

		extern UINT32_T g_schedulerFrequency;

		void SetPITFrequency(UINT32_T freq);
		// A call to SetPITFrequency will enable the PIT once again.
		void DisablePIT();

		bool IsInScheduler();

		void InitializeMultitasking();
	}
}