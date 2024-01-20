/*
	oboskrnl/multitasking/arch.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

// This file defines all platform-specific functions/structures that a port of the kernel to get the scheduler to work.

#include <int.h>
#include <export.h>

#if defined(__x86_64__) || defined(_WIN64)
#include <multitasking/x86_64/arch_structs.h>
#endif

extern "C" void idleTask();

namespace obos
{
#ifndef ALLOCATORS_VMM_VMM_H_INCLUDED
	namespace memory
	{
		class VirtualAllocator;
	}
#endif
	namespace thread
	{
		void switchToThreadImpl(taskSwitchInfo* info, struct Thread* thread);
		bool callBlockCallbackOnThread(taskSwitchInfo* info, bool(*callback)(void* thread, void* userdata), void* par1, void* par2);
		void setupThreadContext(taskSwitchInfo* info, void* stackInfo, uintptr_t entry, uintptr_t userdata, size_t stackSize, memory::VirtualAllocator* vallocator, void* asProc);
		void freeThreadStackInfo(void* stackInfo, memory::VirtualAllocator* vallocator);
		void setupTimerInterrupt();

		OBOS_EXPORT uintptr_t stopTimer();
		OBOS_EXPORT void startTimer(uintptr_t);
		
		OBOS_EXPORT void callScheduler(bool allCores);

		void* getCurrentCpuLocalPtr();
		// For any kernel/driver developers, this does nothing but send the other cores to a trampoline.
		// It DOES NOT undo the action of StopCPUs()
		bool StartCPUs();
		OBOS_EXPORT void StopCPUs(bool includingSelf);

		bool inSchedulerFunction(struct Thread* thr);
	}
}