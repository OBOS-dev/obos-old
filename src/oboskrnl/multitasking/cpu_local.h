/*
	multitasking/cpu_local.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

#include <multitasking/thread.h>
#include <multitasking/arch.h>

namespace obos
{
	namespace thread
	{
		struct cpu_local
		{
			Thread::StackInfo startup_stack{};
			volatile Thread* currentThread = nullptr;
			volatile bool schedulerLock = false;
			uint32_t cpuId = 0;
			bool initialized = false;
			Thread::StackInfo temp_stack{};
			cpu_local_arch arch_specific{};
			bool isBSP = false;
			Thread* idleThread = nullptr;
		};
		extern cpu_local* g_cpuInfo;
		extern size_t g_nCPUs;
		OBOS_EXPORT cpu_local* GetCurrentCpuLocalPtr();
	}
}