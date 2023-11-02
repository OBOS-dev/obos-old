/*
	oboskrnl/multitasking/scheduler.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/arch.h>

namespace obos
{
	extern void kmain_common();
	namespace thread
	{
		Thread::ThreadList g_priorityLists[4];
		volatile Thread* g_currentThread = nullptr;
		uint64_t g_schedulerFrequency = 1000;
		uint64_t g_timerTicks = 0;
		bool g_initialized = false;
		volatile bool g_schedulerLock = false;

#pragma GCC push_options
#pragma GCC optimize("O1")
		Thread* findRunnableThreadInList(Thread::ThreadList& list)
		{
			Thread* currentThread = list.tail;
			Thread* ret = nullptr;

			while (currentThread)
			{
				bool clearTimeSliceIndex = currentThread->status & THREAD_STATUS_CLEAR_TIME_SLICE_INDEX;
				
				if (currentThread->priority == currentThread->timeSliceIndex)
					currentThread->status |= THREAD_STATUS_CLEAR_TIME_SLICE_INDEX;
				
				if (currentThread->status == THREAD_STATUS_CAN_RUN && (currentThread->timeSliceIndex < currentThread->priority))
					if (!ret || currentThread->lastTimePreempted < ret->lastTimePreempted)
						ret = currentThread;

				if (clearTimeSliceIndex)
				{
					currentThread->status &= ~THREAD_STATUS_CLEAR_TIME_SLICE_INDEX;
					currentThread->timeSliceIndex = 0;
				}

				currentThread = currentThread->prev_run;
			}

			return ret;
		}
		Thread::ThreadList& findThreadPriorityList()
		{
			Thread::ThreadList* list = nullptr;
			int i;
			for (i = 3; i > -1; i--)
			{
				constexpr thrPriority priorityTable[4] = {
					THREAD_PRIORITY_IDLE,
					THREAD_PRIORITY_LOW,
					THREAD_PRIORITY_NORMAL,
					THREAD_PRIORITY_HIGH,
				};
				thrPriority priority = priorityTable[i];
				list = &g_priorityLists[i];
				if (list->size > 0)
				{
					if (g_priorityLists[i].iterations < (int)priority)
						break;
				}
			}
			if (g_priorityLists[0].iterations >= (int)THREAD_PRIORITY_IDLE)
			{
				for (int i = 0; i < 4; i++)
					g_priorityLists[i].iterations = 0;
				return findThreadPriorityList();
			}
			g_priorityLists[i].iterations++;
			return *list;
		}
		void callBlockCallbacksOnList(Thread::ThreadList& list)
		{
			Thread* thread = list.head;
			while(thread)
			{
				if (thread->status & THREAD_STATUS_BLOCKED)
				{
					bool ret = callBlockCallbackOnThread(&thread->context, (bool(*)(void*,void*))thread->blockCallback.callback, thread, thread->blockCallback.userdata);
					if (ret)
						thread->status &= ~THREAD_STATUS_BLOCKED;
				}

				thread = thread->next_run;
			}
		}

		void schedule()
		{
			g_timerTicks++;
			g_currentThread->lastTimePreempted = g_timerTicks;
			if (g_schedulerLock)
				return;

			g_schedulerLock = true;

			callBlockCallbacksOnList(g_priorityLists[3]);
			callBlockCallbacksOnList(g_priorityLists[2]);
			callBlockCallbacksOnList(g_priorityLists[1]);
			callBlockCallbacksOnList(g_priorityLists[0]);

			Thread::ThreadList* list = &findThreadPriorityList();
			int foundHighPriority = 0;
			find:
			Thread* newThread = findRunnableThreadInList(*list);
			if (!newThread)
			{
				foundHighPriority += list == &g_priorityLists[3];
				list = list->prevThreadList;
				if(foundHighPriority < 2)
					goto find;
			}
			if (foundHighPriority == 2)
				newThread = g_priorityLists[0].head;
			if (newThread == g_currentThread)
			{
				g_schedulerLock = false;
				return;
			}
			g_currentThread = newThread;
			g_currentThread->timeSliceIndex = g_currentThread->timeSliceIndex + 1;
			g_schedulerLock = false;
			switchToThreadImpl((taskSwitchInfo*)&g_currentThread->context);
		}
#pragma GCC pop_options

		void InitializeScheduler()
		{
			Thread* kernelMainThread  = new Thread{};
			g_currentThread = (volatile Thread*)kernelMainThread;

			kernelMainThread->tid = g_nextTid++;
			kernelMainThread->status = THREAD_STATUS_CAN_RUN;
			kernelMainThread->priority = THREAD_PRIORITY_NORMAL;
			kernelMainThread->exitCode = 0;
			kernelMainThread->lastError = 0;
			kernelMainThread->priorityList = g_priorityLists + 2;
			kernelMainThread->threadList = new Thread::ThreadList;
			setupThreadContext(&kernelMainThread->context, &kernelMainThread->stackInfo, (uintptr_t)kmain_common, 0, 0x8000, false);

			Thread* idleThread = new Thread{};

			idleThread->tid = g_nextTid++;
			idleThread->status = THREAD_STATUS_CAN_RUN;
			idleThread->priority = THREAD_PRIORITY_IDLE;
			idleThread->exitCode = 0;
			idleThread->lastError = 0;
			idleThread->priorityList = g_priorityLists + 0;
			idleThread->threadList = kernelMainThread->threadList;
			setupThreadContext(&idleThread->context, &idleThread->stackInfo, (uintptr_t)idleTask, 0, 4096, false);

			kernelMainThread->threadList->head = kernelMainThread;
			kernelMainThread->threadList->tail = idleThread;
			kernelMainThread->threadList->size = 2;
			kernelMainThread->next_list = idleThread;
			idleThread->prev_list = kernelMainThread;

			g_priorityLists[2].head = g_priorityLists[2].tail = kernelMainThread;
			g_priorityLists[2].size++;
			g_priorityLists[0].head = g_priorityLists[0].tail = idleThread;
			g_priorityLists[0].size++;

			g_priorityLists[3].prevThreadList = g_priorityLists + 2;
			g_priorityLists[2].prevThreadList = g_priorityLists + 1;
			g_priorityLists[1].prevThreadList = g_priorityLists + 0;
			g_priorityLists[0].prevThreadList = g_priorityLists + 3;

			g_priorityLists[3].nextThreadList = g_priorityLists + 0;
			g_priorityLists[2].nextThreadList = g_priorityLists + 3;
			g_priorityLists[1].nextThreadList = g_priorityLists + 2;
			g_priorityLists[0].nextThreadList = g_priorityLists + 1;

			setupTimerInterrupt();
			switchToThreadImpl((taskSwitchInfo*)&g_currentThread->context);
		}
	}
}