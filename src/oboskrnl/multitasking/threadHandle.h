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
			ThreadHandle() = default;
			ThreadHandle(Thread::Tid tid);

			ThreadHandle(const ThreadHandle& other);
			ThreadHandle& operator=(const ThreadHandle& other);
			ThreadHandle(ThreadHandle&& other);
			ThreadHandle& operator=(ThreadHandle&& other);
			
			bool CreateThread(Thread::priority_t threadPriority, DWORD(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus);
			void OpenThread(Thread::Tid tid);
			void PauseThread();
			void ResumeThread();
			void SetThreadPriority(Thread::priority_t newPriority);
			void SetThreadStatus(utils::RawBitfield newStatus);
			void TerminateThread(DWORD exitCode);
			
			Thread::Tid GetTid();
			DWORD GetExitCode();
			Thread::priority_t GetThreadPriority();
			utils::RawBitfield GetThreadStatus();

			constexpr static handleType getType()
			{
				return handleType::thread;
			}

			virtual Handle* duplicate() override;
			virtual int closeHandle() override;

			~ThreadHandle();
		};
	}
}