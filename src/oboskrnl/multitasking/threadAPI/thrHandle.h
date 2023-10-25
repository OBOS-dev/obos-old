/*
	oboskrnl/threadAPI/thrHandle.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace thread
	{
		class ThreadHandle
		{
		public:
			ThreadHandle();
			
			bool OpenThread(uint32_t tid);
			
			bool CreateThread(uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, bool startPaused = false, bool isUsermode = false);

			bool PauseThread();
			bool ResumeThread();
			bool SetThreadPriority(uint32_t priority);
			bool TerminateThread(uint32_t exitCode);

			uint32_t GetThreadStatus();
			uint32_t GetThreadExitCode();
			uint32_t GetThreadLastError();

			bool CloseHandle();

			~ThreadHandle() { CloseHandle(); }
		private:
			void* m_obj = nullptr;
		};

		[[noreturn]] void ExitThread(uint32_t exitCode);
	}
}