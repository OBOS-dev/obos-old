/*
	oboskrnl/multitasking/thread.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/interrupt.h>

#include <multitasking/taskSwitch.h>

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
		};
		enum thrPriority
		{
			THREAD_PRIORITY_IDLE,
			THREAD_PRIORITY_LOW,
			THREAD_PRIORITY_NORMAL,
			THREAD_PRIORITY_HIGH,
		};
		struct Thread
		{
			uint32_t tid;
			uint32_t status;
			uint32_t priority;
			uint32_t exitCode;
			uint32_t lastError;
			struct
			{
				void* addr;
				size_t nPages;
			} stackInfo;
			taskSwitchInfo context;
			Thread* next;
			Thread* prev;
		};
		struct PriorityList
		{
			Thread* head;
			Thread* tail;
			size_t size;
		};
	}
}