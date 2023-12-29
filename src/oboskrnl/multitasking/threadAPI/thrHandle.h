/*
	oboskrnl/threadAPI/thrHandle.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <export.h>

namespace obos
{
	namespace thread
	{
		extern uint64_t g_defaultAffinity;
		class ThreadHandle
		{
		public:
			OBOS_EXPORT ThreadHandle();
			
			/// <summary>
			/// Gets the scheduler's thread object.
			/// </summary>
			/// <returns>The object.</returns>
			OBOS_EXPORT void* GetUnderlyingObject() const { return m_obj; }

			/// <summary>
			/// Opens the thread "tid"
			/// </summary>
			/// <param name="tid">The thread id.</param>
			/// <returns>Whether the function succeeded or not.</returns>
			OBOS_EXPORT bool OpenThread(uint32_t tid);
			
			/// <summary>
			/// Creates a thread.
			/// </summary>
			/// <param name="priority">The new thread's priority.</param>
			/// <param name="stackSize">The stack's size in bytes.</param>
			/// <param name="entry">The thread's entry point.</param>
			/// <param name="userdata">The parameter to pass to the thread.</param>
			/// <param name="process">Which process to create the thread as.</param>
			/// <param name="startPaused">Whether the thread should be paused when it starts.</param>
			/// <returns>Whether the function succeeded or not.</returns>
			OBOS_EXPORT bool CreateThread(uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, uint64_t affinity = g_defaultAffinity, void* process = nullptr, bool startPaused = false);

			/// <summary>
			/// Pauses the thread.
			/// </summary>
			/// <returns>Whether the function succeeded or not.</returns>
			OBOS_EXPORT bool PauseThread();
			/// <summary>
			/// Resumes the thread.
			/// </summary>
			/// <returns>Whether the function succeeded or not.</returns>
			OBOS_EXPORT bool ResumeThread();
			/// <summary>
			/// Changes the thread's priority to "priority"
			/// </summary>
			/// <param name="priority">The new priority.</param>
			/// <returns>Whether the function succeeded or not.</returns>
			OBOS_EXPORT bool SetThreadPriority(uint32_t priority);
			/// <summary>
			/// Terminates the thread.
			/// </summary>
			/// <param name="exitCode">The exit code to exit with.</param>
			/// <returns>Whether the function succeeded or not.</returns>
			OBOS_EXPORT bool TerminateThread(uint32_t exitCode);

			/// <summary>
			/// Gets the thread status.
			/// </summary>
			/// <returns>The thread's status, or zero on failure.</returns>
			OBOS_EXPORT uint32_t GetThreadStatus();
			/// <summary>
			/// Gets the thread's exit code if it exited.
			/// </summary>
			/// <returns>The thread's exit code, or zero on failure.</returns>
			OBOS_EXPORT uint32_t GetThreadExitCode();
			/// <summary>
			/// Gets the thread's last error. This function is equivalent to calling GetLastError() in the thread.
			/// </summary>
			/// <returns>The error code.</returns>
			OBOS_EXPORT uint32_t GetThreadLastError();
			/// <summary>
			/// Gets the thread's id.
			/// </summary>
			/// <returns>The thread's id, or zero on failure.</returns>
			OBOS_EXPORT uint32_t GetThreadTID();

			/// <summary>
			/// Closes the thread handle.
			/// </summary>
			/// <returns>Whether the function succeeded or not.</returns>
			OBOS_EXPORT bool CloseHandle();

			OBOS_EXPORT ~ThreadHandle() { CloseHandle(); }
		private:
			void* m_obj = nullptr;
		};

		/// <summary>
		/// Get the current thread's id.
		/// </summary>
		/// <returns>The current tid.</returns>
		OBOS_EXPORT uint32_t GetTID();

		/// <summary>
		/// Exits the current thread.
		/// </summary>
		/// <param name="exitCode">The thread's exit code.</param>
		OBOS_EXPORT [[noreturn]] void ExitThread(uint32_t exitCode);
	}
}