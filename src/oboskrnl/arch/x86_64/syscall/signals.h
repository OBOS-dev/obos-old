/*
	arch/x86_64/syscall/signals.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/process/signals.h>

#include <arch/x86_64/syscall/handle.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t SignalsSyscallHandler(uint64_t syscall, void* args);

		/// <summary>
		/// Syscall Number: 57<para></para>
		/// Registers a signal handler for the current process.
		/// </summary>
		/// <param name="signal">The signal to register.</param>
		/// <param name="uhandler">The handler, or nullptr to clear the handler.</param>
		/// <returns>Whether the signal was successfully registered (true) or not (false).</returns>
		bool SyscallRegisterSignal(process::signals signal, uintptr_t uhandler);
		/// <summary>
		/// Syscall Number: 58<para></para>
		/// Calls a signal on a thread.
		/// </summary>
		/// <param name="on">A thread handle representing the thread to call the signal on.</param>
		/// <param name="sig">The signal to send.</param>
		/// <returns>Whether the signal was successfully send (true) or not (false).</returns>
		bool SyscallCallSignal(user_handle on, process::signals sig);
	}
}