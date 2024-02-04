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

        /// <summary>
        /// System number: 59<para></para>
        /// Shuts down the system, doing necessary cleanup.<para></para>
        /// If for some reason this fails, all CPUs are stopped and the system idles.<para></para>
        /// This syscall does not return.
        /// </summary>
        void SyscallShutdown();
        /// <summary>
        /// System number: 60<para></para>
        /// Reboots the system, doing necessary cleanup.<para></para>
        /// If for some reason this fails, all CPUs are stopped and the system idles.<para></para>
        /// This syscall does not return.
        /// </summary>
        void SyscallReboot();
    } // namespace syscall
    
} // namespace obos
