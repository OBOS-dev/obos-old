/*
    oboskrnl/multitasking/process/signals.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <multitasking/thread.h>

namespace obos
{
    namespace process
    {
        // See documentation/signals.md for documentation on these signals.
        enum signals
        {
            SIGPF,
            SIGPM,
            SIGOF,
            SIGME,
            SIGDG,
            SIGTIME,
            SIGTERM,
            SIGINT,
            SIGUDOC,
            SIGUEXC,

            SIGMAX = SIGUDOC,
            INVALID_SIGNAL,
        };

        bool CallSignal(thread::Thread* on, signals sig);
        bool CallSignalOrTerminate(thread::Thread* on, signals sig);
    }
}