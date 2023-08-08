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
			/// <returns></returns>
			bool CreateThread(Thread::priority_t threadPriority, VOID(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus, SIZE_T stackSizePages);
			bool OpenThread(Thread::Tid tid);
			bool OpenThread(Thread* thread);
			void PauseThread(bool force = false);
			void ResumeThread();
			void SetThreadPriority(Thread::priority_t newPriority);
			void SetThreadStatus(utils::RawBitfield newStatus);
			// Cannot be used on the current thread.
			void TerminateThread(DWORD exitCode);
			
			Thread::Tid GetTid();
			DWORD GetExitCode();
			Thread::priority_t GetThreadPriority();
			utils::RawBitfield GetThreadStatus();

			constexpr static handleType getType()
			{
				return handleType::thread;
			}

			Handle* duplicate() override;
			int closeHandle() override;

			~ThreadHandle() { closeHandle(); }

			friend void ExitThread(DWORD exitCode);
		private:
			Thread* m_thread = nullptr;
			using status_t = Thread::status_t;
		};

		void ExitThread(DWORD exitCode);
		DWORD GetCurrentThreadTid();
		ThreadHandle* GetCurrentThreadHandle();
		Thread::priority_t GetCurrentThreadPriority();
		utils::RawBitfield GetCurrentThreadStatus();
	}
}