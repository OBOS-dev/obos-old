/*
	oboskrnl/multitasking/arch.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

// This file defines all platform-specific functions/structures that a port of the kernel to get the scheduler to work.

#include <arch/interrupt.h>

extern "C" void idleTask();

namespace obos
{
	namespace thread
	{
#if defined(__x86_64__)
#include <multitasking/x86_64/taskSwitchInfo.h>
#endif
		void switchToThreadImpl(taskSwitchInfo* info);
		bool callBlockCallbackOnThread(taskSwitchInfo* info, bool(*callback)(void* thread, void* userdata), void* par1, void* par2);
		void setupThreadContext(taskSwitchInfo* info, uintptr_t entry, uintptr_t userdata, size_t stackSize, bool isUsermodeProgram);
		void setupTimerInterrupt();

		uintptr_t stopTimer();
		void startTimer(uintptr_t);
		
		void callScheduler();
	}
}