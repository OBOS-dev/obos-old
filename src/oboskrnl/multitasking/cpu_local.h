/*
	multitasking/cpu_local.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/thread.h>

namespace obos
{
	namespace thread
	{
		struct cpu_local
		{
			Thread::StackInfo startup_stack;
			volatile Thread* currentThread;
			volatile bool schedulerLock;
			uint32_t cpuId;
			bool initialized;
		};
		extern cpu_local* g_cpuInfo;
		extern size_t g_nCPUs;
		cpu_local* GetCurrentCpuLocalPtr();
	}
}