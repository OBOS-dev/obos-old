/*
	thread.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>
#include <klog.h>
#include <inline-asm.h>

#include <utils/bitfields.h>

#include <descriptors/idt/idt.h>

#include <process/process.h>

namespace obos
{
	namespace multitasking
	{
		extern UINT32_T g_nextThreadTid;

		class Thread final
		{
		public:
			enum class priority_t
			{
				INVALID_PRIORITY,
				// An idle thread.
				IDLE = 2,
				// A low-priority thread.
				LOW = 3,
				// A thread of normal priority.
				NORMAL = 4,
				// A high-priority thread.
				HIGH = 8
			};
			enum class status_t
			{
				DEAD = 1,
				RUNNING = 2,
				BLOCKED = 4,
				PAUSED = 8
			};
			using Tid = DWORD;
		public:
			Thread() {
				nop();
			};
			Thread(priority_t threadPriority, VOID(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus, SIZE_T stackSizePages);

			DWORD CreateThread(priority_t threadPriority, VOID(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus, SIZE_T stackSizePages);

			//void SetThreadPriority(priority_t newPriority)
			//{
			//	priority = newPriority;
			//}
			//void SetThreadStatus(utils::RawBitfield newStatus)
			//{
			//	status = newStatus;
			//}
			//priority_t GetThreadPriority()
			//{
			//	return priority;
			//}
			//utils::RawBitfield GetThreadStatus()
			//{
			//	return status;
			//}
			//Tid GetTid()
			//{
			//	return tid;
			//}
			//DWORD GetExitCode()
			//{
			//	return exitCode;
			//}

			virtual ~Thread()
			{
				if (status != (UINT32_T)status_t::DEAD && tid != ((DWORD)-1))
					kpanic(nullptr, getEIP(), kpanic_format("A thread object was destroyed without the thread being dead."));
			}

			/*friend void InitializeMultitasking();
			friend void findNewTask(const interrupt_frame* frame);
			friend class ThreadHandle;*/
		public:
			Tid tid = -1;
			DWORD exitCode = 0;
			DWORD lastError = 0;
			priority_t priority = priority_t::IDLE;
			utils::RawBitfield status = 0;
			bool(*isBlockedCallback)(Thread* _this, PVOID userData) = nullptr;
			PVOID isBlockedUserdata = nullptr;
			interrupt_frame frame;
			SIZE_T iterations = 0;
			SIZE_T nHandles = 0;
			PVOID stackBottom = nullptr;
			SIZE_T stackSizePages = 0;
			process::Process* owner = nullptr;
			PVOID tssStackBottom = nullptr; // If allocated, it will be 2 pages.
			bool isServicingSyscall = false;
			SIZE_T wakeUpTime = 0;
			SIZE_T lastTimeRan = 0;
		};
	}
}