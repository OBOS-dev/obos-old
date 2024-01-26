/*
	oboskrnl/multitasking/thread.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/arch.h>

#define MULTIASKING_THREAD_H_INCLUDED

#if defined(_GNUC_)
#define OBOS_ALIGN(n) __attribute__((aligned(n))
#else
#define OBOS_ALIGN(n)
#endif

namespace obos
{
	namespace thread
	{
		enum thrStatus
		{
			THREAD_STATUS_DEAD = 0x01,
			THREAD_STATUS_CAN_RUN = 0x02,
			THREAD_STATUS_BLOCKED = 0x04,
			THREAD_STATUS_PAUSED = 0x08,
			THREAD_STATUS_CLEAR_TIME_SLICE_INDEX = 0x10,
			THREAD_STATUS_RUNNING = 0x20,
		};
		enum thrFlags
		{
			THREAD_FLAGS_IN_SIGNAL = 0x01,
			THREAD_FLAGS_SINGLE_STEPPING = 0x02,
			THREAD_FLAGS_CALLING_BLOCK_CALLBACK = 0x04,
			THREAD_FLAGS_IS_EXITING_PROCESS = 0x08,
		};
		enum thrPriority
		{
			THREAD_PRIORITY_IDLE = 1,
			THREAD_PRIORITY_LOW = 2,
			THREAD_PRIORITY_NORMAL = 4,
			THREAD_PRIORITY_HIGH = 8,
		};
		struct Thread
		{
			struct ThreadList
			{
				Thread* head;
				Thread* tail;
				size_t size;
				ThreadList* nextThreadList;
				ThreadList* prevThreadList;
				size_t iterations; // How many times the scheduler has run the list since the last iterations clear.
				bool lock;
			};
			uint32_t tid;
			uint32_t status;
			uint32_t priority;
			uint32_t timeSliceIndex;
			uint32_t exitCode;
			uint64_t lastTimePreempted;
			uint64_t wakeUpTime;
			uint32_t lastError;
			uint32_t references;
			struct
			{
				bool(*callback)(Thread* _this, void* userData);
				void* userdata;
			} blockCallback;
			struct StackInfo
			{
				void* addr;
				size_t size;
			} stackInfo;
			void* owner; // a Process*
			Thread* next_run; // The next in the priority list.
			Thread* prev_run; // The previous in the priority list.
			Thread* next_list; // The next in the process thread list.
			Thread* prev_list;  // The previous in the process thread list.
			ThreadList* priorityList; // A pointer to the priority list.
			ThreadList* threadList; // A pointer to the process' thread list.
			taskSwitchInfo context;
			// If a bit is set, the cpu corresponding to that bit number can run the thread.
			// This limits the kernel to 128 cores.
			__uint128_t affinity, ogAffinity;
			uint32_t flags;
			void* driverIdentity = nullptr;
		} OBOS_ALIGN(4);
	};
}