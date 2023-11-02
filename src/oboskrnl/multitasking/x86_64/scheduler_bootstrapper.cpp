/*
	oboskrnl/multitasking/x86_64/scheduler_bootstrapper.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <arch/interrupt.h>

#include <arch/x86_64/irq/timer.h>
#include <arch/x86_64/irq/irq.h>

#include <multitasking/scheduler.h>

#include <x86_64-utils/asm.h>

#define MISC_ENABLE 0x1a0

extern "C" uint64_t calibrateTimer(uint64_t freq);

namespace obos
{
	namespace thread
	{
		extern void schedule();

		// Assumes the timer divisor is one.
		uint64_t FindCounterValueFromFrequency(uint64_t freq)
		{
			logger::log("%s: Calibrating the timer at a frequency of %d.\n", __func__, freq);
			uint64_t ret = calibrateTimer(freq);
			logger::log("%s: Timer count is %d.\n", __func__, ret);
			return ret;
		}

		extern bool g_schedulerLock;

		void scheduler_bootstrap(interrupt_frame* frame)
		{
			if (!g_initialized || !g_currentThread || g_schedulerLock)
				return;
			utils::dwMemcpy((uint32_t*)&g_currentThread->context.frame, (uint32_t*)frame, sizeof(interrupt_frame) / 4);
			schedule();
		}


		void setupTimerInterrupt()
		{
			uint64_t counter = FindCounterValueFromFrequency(g_schedulerFrequency);

			ConfigureAPICTimer(scheduler_bootstrap, 0x20, counter, TIMER_CONFIG_PERIODIC, TIMER_DIVISOR_ONE);
			RegisterInterruptHandler(0x30, scheduler_bootstrap);
		}
		uintptr_t stopTimer()
		{
			uintptr_t ret = saveFlagsAndCLI();
			if(g_localAPICAddr)
				MaskTimer(false);
			return ret;
		}
		void startTimer(uintptr_t flags)
		{
			if (g_localAPICAddr)
				MaskTimer(true);
			restorePreviousInterruptStatus(flags);
		}
		void callScheduler()
		{
			asm volatile("int $0x30");
		}
	}
}