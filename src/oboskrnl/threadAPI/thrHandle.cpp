/*
	oboskrnl/threadAPI/thrHandle.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <threadAPI/thrHandle.h>

#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/arch.h>

namespace obos
{
	namespace thread
	{
		uint32_t g_nextTid = 0;
		void* lookForThreadInList(Thread::ThreadList& list, uint32_t tid)
		{
			Thread* ret = list.head;
			while (ret)
			{
				if (ret->tid == tid)
					return ret;

				ret = ret->next_run;
			}
			return nullptr;
		}
		ThreadHandle::ThreadHandle() 
			: m_obj{ nullptr }
		{}
		
		bool ThreadHandle::OpenThread(uint32_t tid)
		{
			void* obj = nullptr;
			obj = lookForThreadInList(g_priorityLists[0], tid);
			if(!obj)
				obj = lookForThreadInList(g_priorityLists[1], tid);
			if (!obj)
				obj = lookForThreadInList(g_priorityLists[2], tid);
			if (!obj)
				obj = lookForThreadInList(g_priorityLists[3], tid);
			if (!obj)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return false;
			}
			return true;
		}

		bool ThreadHandle::CreateThread(uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, bool startPaused, bool isUsermode)
		{
			if (priority != THREAD_PRIORITY_IDLE && priority != THREAD_PRIORITY_LOW && priority != THREAD_PRIORITY_NORMAL && priority != THREAD_PRIORITY_HIGH)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETERS);
				return false;
			}
			Thread::ThreadList* priorityList;
			switch (priority)
			{
			case THREAD_PRIORITY_IDLE:
				priorityList = g_priorityLists + 0;
				break;
			case THREAD_PRIORITY_LOW:
				priorityList = g_priorityLists + 1;
				break;
			case THREAD_PRIORITY_NORMAL:
				priorityList = g_priorityLists + 2;
				break;
			case THREAD_PRIORITY_HIGH:
				priorityList = g_priorityLists + 3;
				break;
			default:
				break;
			}
			Thread* thread = new Thread{};
			
			thread->tid = g_nextTid++;
			thread->status = THREAD_STATUS_CAN_RUN | (startPaused ? THREAD_STATUS_PAUSED : 0);
			thread->priority = THREAD_PRIORITY_NORMAL;
			thread->exitCode = 0;
			thread->lastError = 0;
			thread->priorityList = priorityList;
			thread->threadList = g_currentThread->threadList;
			setupThreadContext(&thread->context, &thread->stackInfo, (uintptr_t)entry, userdata, stackSize, isUsermode);

			uintptr_t val = stopTimer();

			if(priorityList->tail)
			{
				priorityList->tail->next_run = thread;
				thread->prev_run = priorityList->tail;
			}
			if (!priorityList->head)
				priorityList->head = thread;
			priorityList->tail = thread;
			priorityList->size++;

			startTimer(val);

			return true;
		}
	}
}