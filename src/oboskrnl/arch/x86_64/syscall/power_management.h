/*
    oboskrnl/arch/x86_64/syscall/power_management.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
    namespace syscalls
    {
        uintptr_t PMSyscallHandler(uint64_t syscall, void* args);

        void SyscallShutdown();
        void SyscallReboot();
    } // namespace syscall
    
} // namespace obos
