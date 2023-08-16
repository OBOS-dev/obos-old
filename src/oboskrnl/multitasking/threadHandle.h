/*
	threadHandle.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <handle.h>
#include <multitasking/thread.h>

namespace obos
{
	namespace multitasking
	{
		class ThreadHandle final : public Handle
		{
		public:
			/// <summary>
			/// Constructs the handle.
			/// </summary>
			ThreadHandle();

			ThreadHandle(const ThreadHandle& other) = delete;
			ThreadHandle& operator=(const ThreadHandle& other) = delete;
			ThreadHandle(ThreadHandle&& other) = delete;
			ThreadHandle& operator=(ThreadHandle&& other) = delete;
			
			/// <summary>
			/// Creates a thread.
			/// </summary>
			/// <param name="threadPriority">- The priority.</param>
			/// <param name="entry">- The entry point of the thread.</param>
			/// <param name="userData">- The user-defined parameter for the thread.</param>
			/// <param name="threadStatus">- The status of the thread. THREAD_DEAD and THREAD_BLOCKED are ignored.</param>
			/// <param name="stackSizePages">The size of the thread's stack. If this is zero, it will set to two.</param>
			/// <returns>false on failure, otherwise true.</returns>
			bool CreateThread(Thread::priority_t threadPriority, VOID(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus, SIZE_T stackSizePages);
			/// <summary>
			/// Opens a thread handle based on it's tid.
			/// </summary>
			/// <param name="tid">- The thread's tid.</param>
			/// <returns>false on failure, otherwise true.</returns>
			bool OpenThread(Thread::Tid tid);
			/// <summary>
			/// Opens a thread handle based on it's class Thread object.
			/// </summary>
			/// <param name="thread">- The thread's object.</param>
			/// <returns>false on failure, otherwise true.</returns>
			bool OpenThread(Thread* thread);
			/// <summary>
			/// Pauses the thread.
			/// </summary>
			/// <param name="force">- Whether to pause the thread even it is the current thread.</param>
			void PauseThread(bool force = false);
			/// <summary>
			/// Resumes the thread.
			/// </summary>
			void ResumeThread();
			/// <summary>
			/// Sets the thread's priority.
			/// </summary>
			/// <param name="newPriority">- The new priority.</param>
			void SetThreadPriority(Thread::priority_t newPriority);
			/// <summary>
			/// Set's the thread's status. This cannot be THREAD_DEAD or THREAD_BLOCKED.
			/// </summary>
			/// <param name="newStatus">- The new status.</param>
			void SetThreadStatus(utils::RawBitfield newStatus);
			/// <summary>
			/// Terminates the thread. This cannot be used on the current thread.
			/// </summary>
			/// <param name="exitCode">- The exit code.</param>
			void TerminateThread(DWORD exitCode);
			
			/// <summary>
			/// Gets the thread tid.
			/// </summary>
			/// <returns>The threads tid, or -1 on failure.</returns>
			Thread::Tid GetTid();
			/// <summary>
			/// Gets the thread's exit code if it exited.
			/// </summary>
			/// <returns>The exit code. If the function fails it returns zero.</returns>
			DWORD GetExitCode();
			/// <summary>
			/// Gets the thread's priority.
			/// </summary>
			/// <returns>The thread's priority, or, failure, zero.</returns>
			Thread::priority_t GetThreadPriority();
			/// <summary>
			/// Gets the thread's status.
			/// </summary>
			/// <returns>The thread's status, or, failure, zero.</returns>
			utils::RawBitfield GetThreadStatus();

			/// <summary>
			/// Gets the type of the handle.
			/// </summary>
			/// <returns>handleType::thread.</returns>
			constexpr static handleType getType()
			{
				return handleType::thread;
			}

			/// <summary>
			/// Waits for the thread to exit.
			/// </summary>
			/// <returns>false is the handle wasn't opened, otherwise true.</returns>
			bool WaitForThreadExit();
			/// <summary>
			/// Waits for the thread to change it's status to.
			/// </summary>
			/// <returns>false is the handle wasn't opened, otherwise true.</returns>
			bool WaitForThreadStatusChange(utils::RawBitfield newStatus);

			/// <summary>
			/// Duplicates the thread handle.
			/// </summary>
			/// <returns>The new thread handle, or on failure, nullptr.</returns>
			Handle* duplicate() override;
			/// <summary>
			/// Closes the handle.
			/// </summary>
			/// <returns>The number of references to this thread.</returns>
			int closeHandle() override;

			~ThreadHandle() { closeHandle(); }

			friend void ExitThread(DWORD exitCode);
		private:
			Thread* m_thread = nullptr;
			using status_t = Thread::status_t;
		};

		/// <summary>
		/// Exits the current thread.
		/// </summary>
		/// <param name="exitCode">- The exit code.</param>
		void ExitThread(DWORD exitCode);
		/// <summary>
		/// Gets the current thread's tid.
		/// </summary>
		/// <returns>The tid.</returns>
		DWORD GetCurrentThreadTid();
		/// <summary>
		/// Makes a handle based on the current thread.
		/// </summary>
		/// <returns>The new handle.</returns>
		ThreadHandle* GetCurrentThreadHandle();
		/// <summary>
		/// Gets the current thread's priority.
		/// </summary>
		/// <returns>The priority.</returns>
		Thread::priority_t GetCurrentThreadPriority();
		/// <summary>
		/// Gets the current's status.
		/// </summary>
		/// <returns>A bitfield representing the status.</returns>
		utils::RawBitfield GetCurrentThreadStatus();
	}
}