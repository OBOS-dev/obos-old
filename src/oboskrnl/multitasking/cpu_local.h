/*
	multitasking/cpu_local.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/thread.h>
#include <multitasking/arch.h>

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
			Thread::StackInfo temp_stack;
			cpu_local_arch arch_specific;
		};
		extern cpu_local* g_cpuInfo;
		extern size_t g_nCPUs;
		cpu_local* GetCurrentCpuLocalPtr();
	}
}