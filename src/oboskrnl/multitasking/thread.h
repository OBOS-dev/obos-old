/*
	oboskrnl/multitasking/thread.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/arch.h>

#define MULTIASKING_THREAD_H_INCLUDED

namespace obos
{
	namespace thread
	{
		enum thrStatus
		{
			THREAD_STATUS_DEAD = 0b1,
			THREAD_STATUS_CAN_RUN = 0b10,
			THREAD_STATUS_BLOCKED = 0b100,
			THREAD_STATUS_PAUSED = 0b1000,
			THREAD_STATUS_CLEAR_TIME_SLICE_INDEX = 0b10000,
			THREAD_STATUS_RUNNING = 0b100000,
			THREAD_STATUS_CALLING_BLOCK_CALLBACK = 0b1000000,
			THREAD_STATUS_SINGLE_STEPPING = 0b10000000, // Used for debugging.
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
			taskSwitchInfo context;
			void* owner; // a Process*
			Thread* next_run; // The next in the priority list.
			Thread* prev_run; // The previous in the priority list.
			Thread* next_list; // The next in the process thread list.
			Thread* prev_list;  // The previous in the process thread list.
			ThreadList* priorityList; // A pointer to the priority list.
			ThreadList* threadList; // A pointer to the process' thread list.
		};
	}
}