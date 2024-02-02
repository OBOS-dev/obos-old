/*
    oboskrnl/arch/x86_64/signals.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <multitasking/thread.h>
#include <multitasking/cpu_local.h>

#include <multitasking/process/process.h>
#include <multitasking/process/signals.h>

#include <arch/x86_64/irq/irq.h>

#include <x86_64-utils/asm.h>

namespace obos
{
    namespace process
    {
        template<typename i>
        static bool testbit(i bitfield, uint8_t bit)
        {
            return (bitfield >> bit) & 1;
        }
        void __ImplCallSignal(thread::Thread* on, void(*handler)())
        {
            on->flags |= thread::THREAD_FLAGS_IN_SIGNAL;
            bool isRunning = on->status & thread::THREAD_STATUS_RUNNING;
            __uint128_t affinity = on->affinity;
            on->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_PAUSED;
            if (isRunning)
            {
                // Find the cpu the thread is running on.
                uint32_t cpuId = bsf(affinity);
                // Call the scheduler on the cpu.
                SendIPI(DestinationShorthand::None, DeliveryMode::Fixed, 0x30, cpuId);
            }
            uintptr_t previousRip = on->context.frame.rip;
            on->context.frame.rip = (uintptr_t)handler;
            uintptr_t* returnAddress = (uintptr_t*)(on->context.frame.rsp -= 8);
            setAC();
            *returnAddress = previousRip;
            clearAC();
            on->status = thread::THREAD_STATUS_CAN_RUN;
        }

        bool CallSignal(thread::Thread* on, signals sig)
        {
            if (sig > SIGMAX)
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            logger::debug("Calling signal %d on thread %d, process id %d. Action on fail: Abort.\n", sig, on->tid, ((process::Process*)on->owner)->pid);
            auto process = (Process*)on->owner;
            auto handler = process->signal_table[sig];
            if (!handler)
            {
                SetLastError(OBOS_ERROR_NULL_HANDLER);
                return false;
            }
            __ImplCallSignal(on, handler);
            return true;
        }
        bool CallSignalOrTerminate(thread::Thread* on, signals sig)
        {
            if (sig > SIGMAX)
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            logger::debug("Calling signal %d on thread %d, process id %d. Action on fail: Terminate thread's process.\n", sig, on->tid, ((process::Process*)on->owner)->pid);
            auto process = (Process*)on->owner;
            auto handler = process->signal_table[sig];
            if (handler)
                __ImplCallSignal(on, handler);
            else
                TerminateProcess(process);
            return true;
        }
    }
}