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

namespace obos
{
	namespace thread
	{
		extern void schedule();

		static uint32_t getPITCount()
		{
			uint32_t ret = 0;

			outb(0x43, 0b1110010);

			ret = inb(0x40);
			ret |= inb(0x40) << 8;

			return ret;
		}

		// Assumes the timer divisor is one.
		uint64_t FindCounterValueFromFrequency(uint64_t freq)
		{
			// Try cpuid 0x15.
			uint64_t maxCpuid = 0;
			uint64_t ret = 0;
			uint64_t unused = 0;
			__cpuid__(0, 0, &maxCpuid, &unused, &unused, &unused);
			if(maxCpuid >= 0x15)
			{
				logger::info("%s: CPUID 0x15 Supported!\n%sWe don't have to do... sketchy ways of finding the initial count for a timer frequency of %d hz.\n",
					__func__, 
					logger::INFO_PREFIX_MESSAGE,
					freq);
				__cpuid__(0x15, 0, &unused, &unused, &ret, &unused);
				return ret / freq;
			}
			// No cpuid 0x15 :(
			// Give the APIC a high initial counter (0xffffffff).
			// Set the PIT to the frequency specified, poll until it reaches zero.
			// Then substract the APIC's current counter from the initial count.
			logger::info("%s: CPUID 0x15 isn't Supported :(\n%sWe have to do... sketchy ways of finding the initial count for a timer frequency of %d hz.\n", 
				__func__, 
				logger::INFO_PREFIX_MESSAGE, 
				freq);
			MaskTimer(false);
			
			uint64_t flags = saveFlagsAndCLI();

			// Initialize the PIT.

			uint64_t divisor = 1193182 / freq;

			outb(0x43, 0x30);

			uint8_t l = (uint8_t)(divisor & 0xFF);
			uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

			outb(0x40, l);
			outb(0x40, h);

			g_localAPICAddr->divideConfig = TIMER_DIVISOR_ONE;
			g_localAPICAddr->initialCount = 0xffffffff;

			while (getPITCount());

			ret = static_cast<uint64_t>(0xffffffff) - g_localAPICAddr->currentCount;

			g_localAPICAddr->initialCount = 0;

			MaskTimer(true);
			restorePreviousInterruptStatus(flags);

			logger::info("%s: Counter value is %d.\n", __func__, ret);

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
			MaskTimer(false);
			return saveFlagsAndCLI();
		}
		void startTimer(uintptr_t flags)
		{
			restorePreviousInterruptStatus(flags);
			MaskTimer(true);
		}
		void callScheduler()
		{
			asm volatile("int $0x30");
		}
	}
}