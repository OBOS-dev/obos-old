/*
    oboskrnl/arch/x86_64/syscall/power_management.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

namespace obos
{
    void EarlyKPanic();
	bool EnterSleepState(int sleepState);
    namespace syscalls
    {
        uintptr_t PMSyscallHandler(uint64_t syscall, void* args)
        {
            switch (syscall)
            {
            case 59:
                SyscallShutdown();
                break;
            case 60:
                SyscallReboot();
                break;
            default:
                break;
            }
        }

        void SyscallShutdown()
        {
            EnterSleepState(5);
            while(1);
        }
        void SyscallReboot()
        {
            EarlyKPanic();
            while(1);
        }
    } // namespace syscall
    
} // namespace obos
