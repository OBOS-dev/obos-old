/*
	arch/x86_64/syscall/thread.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/x86_64/syscall/handle.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t ThreadSyscallHandler(uint64_t syscall, void* args);

		/// <summary>
		/// Syscall Number: 0
		/// Makes a raw thread handle.
		/// </summary>
		/// <returns>The handle.</returns>
		user_handle SyscallMakeThreadHandle();
		
		/// <summary>
		/// Syscall Number: 1
		/// Opens a thread on the thread handle.
		/// </summary>
		/// <param name="hnd">The thread handle.</param>
		/// <param name="tid">The thread id to open.</param>
		/// <returns></returns>
		bool SyscallOpenThread(user_handle hnd, uint32_t tid);

		/// <summary>
		/// Syscall Number: 2
		/// Creates a new thread.
		/// </summary>
		/// <param name="hnd">The handle to create on.</param>
		/// <param name="priority">The new thread's priority.</param>
		/// <param name="stackSize">The stack's size in bytes.</param>
		/// <param name="entry">The thread's entry point.</param>
		/// <param name="userdata">The parameter to pass to the thread.</param>
		/// <param name="process">Which process to create the thread as.</param>
		/// <param name="startPaused">Whether the thread should be paused when it starts.</param>
		/// <returns>Whether the function succeeded or not.</returns>
		bool SyscallCreateThread(user_handle hnd, uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, uint64_t affinity, void* process, bool startPaused);

		/// <summary>
		/// Syscall Number: 3
		/// Pauses a thread.
		/// </summary>
		/// <param name="hnd">The handle of the thread to pause.</param>
		/// <returns>Whether the function succeeded or not.</returns>
		bool SyscallPauseThread(user_handle hnd);
		/// <summary>
		/// Syscall Number: 4
		/// Resumes a thread.
		/// </summary>
		/// <param name="hnd">The handle of the thread to resume.</param>
		/// <returns>Whether the function succeeded or not.</returns>
		bool SyscallResumeThread(user_handle hnd);
		/// <summary>
		/// Syscall Number: 5
		/// Sets the priority of the thread represented by the handle.
		/// </summary>
		/// <param name="hnd">The handle of the thread to set the priority of.</param>
		/// <param name="priority">The new priority.</param>
		/// <returns>Whether the function succeeded or not.</returns>
		bool SyscallSetThreadPriority(user_handle hnd, uint32_t priority);
		/// <summary>
		/// Syscall Number: 6
		/// Terminates the thread. The thread handle is NOT invalidated by this function.
		/// </summary>
		/// <param name="hnd">The thread to terminate.</param>
		/// <param name="exitCode">The exit code.</param>
		/// <returns>Whether the function succeeded or not.</returns>
		bool SyscallTerminateThread(user_handle hnd, uint32_t exitCode);

		/// <summary>
		/// Syscall Number: 7
		/// Gets the status of the thread.
		/// </summary>
		/// <param name="hnd">The thread handle to query.</param>
		/// <returns>The status, or 0xffffffff on failure.</returns>
		uint32_t SyscallGetThreadStatus(user_handle hnd);
		/// <summary>
		/// Syscall Number: 8
		/// Gets the exit code of the thread.
		/// </summary>
		/// <param name="hnd">The thread handle to query.</param>
		/// <returns>The exit code, or 0xffffffff on failure.</returns>
		uint32_t SyscallGetThreadExitCode(user_handle hnd);
		/// <summary>
		/// Syscall Number: 9
		/// Gets the last error of the thread.
		/// </summary>
		/// <param name="hnd">The thread handle to query.</param>
		/// <returns>The last error of the thread, or 0xffffffff on failure.</returns>
		uint32_t SyscallGetThreadLastError(user_handle hnd);
		/// <summary>
		/// Syscall Number: 10
		/// Gets the thread id of the thread handle.
		/// </summary>
		/// <param name="hnd">The thread handle to query.</param>
		/// <returns>The tid, or 0xffffffff on failure.</returns>
		uint32_t SyscallGetThreadTID(user_handle hnd);

		/// <summary>
		/// Gets the thread id of the calling thread.
		/// </summary>
		/// <returns>The thread id.</returns>
		uint32_t GetCurrentTID();

		/// <summary>
		/// Exits the current thread.
		/// </summary>
		/// <param name="exitCode">The exit code to exit with.</param>
		[[noreturn]] void ExitThread(uint32_t exitCode);
	}
}