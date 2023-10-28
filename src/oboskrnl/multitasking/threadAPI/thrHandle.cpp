/*
	oboskrnl/threadAPI/thrHandle.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <multitasking/threadAPI/thrHandle.h>
#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/arch.h>

#define GET_THREAD reinterpret_cast<obos::thread::Thread*>(m_obj)

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

		bool ThreadHandle::CreateThread(uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, void* threadList, bool startPaused, bool isUsermode)
		{
			if (m_obj)
			{
				SetLastError(OBOS_ERROR_ALREADY_EXISTS);
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
				SetLastError(OBOS_ERROR_INVALID_PARAMETERS);
				return false;
				break;
			}
			Thread* thread = new Thread{};
			
			m_obj = thread;

			thread->tid = g_nextTid++;
			thread->status = THREAD_STATUS_CAN_RUN | (startPaused ? THREAD_STATUS_PAUSED : 0);
			thread->priority = THREAD_PRIORITY_NORMAL;
			thread->exitCode = 0;
			thread->lastError = 0;
			thread->priorityList = priorityList;
			thread->threadList = threadList ? (thread::Thread::ThreadList*)threadList : g_currentThread->threadList;
			thread->owner = g_currentThread->owner;
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
			
			if(thread->threadList->tail)
			{
				thread->threadList->tail->next_run = thread;
				thread->prev_run = thread->threadList->tail;
			}
			if (!thread->threadList->head)
				thread->threadList->head = thread;
			thread->threadList->tail = thread;
			thread->threadList->size++;

			startTimer(val);

			return true;
		}

		bool ThreadHandle::PauseThread()
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			Thread* obj = GET_THREAD;
			if (obj->status == THREAD_STATUS_DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}

			obj->status |= THREAD_STATUS_PAUSED;

			return true;
		}
		bool ThreadHandle::ResumeThread()
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			Thread* obj = GET_THREAD;
			if (obj->status == THREAD_STATUS_DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}

			obj->status &= ~THREAD_STATUS_PAUSED;

			return true;
		}
		bool ThreadHandle::SetThreadPriority(uint32_t priority)
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			Thread* obj = GET_THREAD;
			if (obj->status == THREAD_STATUS_DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}
			if (obj->priority == priority)
				return true;

			Thread::ThreadList* newPriorityList = nullptr;
			switch (priority)
			{
			case THREAD_PRIORITY_IDLE:
				newPriorityList = g_priorityLists + 0;
				break;
			case THREAD_PRIORITY_LOW:
				newPriorityList = g_priorityLists + 1;
				break;
			case THREAD_PRIORITY_NORMAL:
				newPriorityList = g_priorityLists + 2;
				break;
			case THREAD_PRIORITY_HIGH:
				newPriorityList = g_priorityLists + 3;
				break;
			default:
				SetLastError(OBOS_ERROR_INVALID_PARAMETERS);
				return false;
				break;
			}
			Thread::ThreadList* oldPriorityList = obj->priorityList;

			uintptr_t val = stopTimer();

			if (obj->prev_run)
				obj->prev_run->next_run = obj->next_run;
			if (obj->next_run)
				obj->next_run->prev_run = obj->prev_run;
			if (oldPriorityList->head == obj)
				oldPriorityList->head = obj->next_run;
			if (oldPriorityList->tail == obj)
				oldPriorityList->tail = obj->prev_run;
			oldPriorityList->size--;

			if (newPriorityList->tail)
			{
				newPriorityList->tail->next_run = obj;
				obj->prev_run = newPriorityList->tail;
			}
			if (!newPriorityList->head)
				newPriorityList->head = obj;
			newPriorityList->tail = obj;
			newPriorityList->size++;

			obj->priorityList = newPriorityList;
			obj->priority = priority;

			startTimer(val);

			return true;
		}
		bool ThreadHandle::TerminateThread(uint32_t exitCode)
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			Thread* obj = GET_THREAD;
			if (obj->status == THREAD_STATUS_DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}
			if (obj == g_currentThread)
				ExitThread(exitCode);

			obj->status = THREAD_STATUS_DEAD;
			obj->exitCode = exitCode;
			freeThreadStackInfo(&obj->stackInfo);

			uint32_t val = stopTimer();
			
			if (obj->prev_run)
				obj->prev_run->next_run = obj->next_run;
			if (obj->next_run)
				obj->next_run->prev_run = obj->prev_run;
			if (obj->priorityList->head == obj)
				obj->priorityList->head = obj->next_run;
			if (obj->priorityList->tail == obj)
				obj->priorityList->tail = obj->prev_run;
			obj->priorityList->size--;

			startTimer(val);

			return true;
		}
		uint32_t ThreadHandle::GetThreadStatus()
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return 0;
			}
			Thread* obj = GET_THREAD;
			return obj->status;
		}
		uint32_t ThreadHandle::GetThreadExitCode()
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return 0;
			}
			Thread* obj = GET_THREAD;
			return obj->exitCode;
		}
		uint32_t ThreadHandle::GetThreadLastError()
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return 0;
			}
			Thread* obj = GET_THREAD;
			return obj->lastError;
		}

		bool ThreadHandle::CloseHandle()
		{
			if (!m_obj)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			Thread* obj = GET_THREAD;
			if (obj->status == THREAD_STATUS_DEAD && !(--obj->references))
			{
				uint32_t val = stopTimer();
				if (obj->prev_list)
					obj->prev_list->next_list= obj->next_list;
				if (obj->prev_list)
					obj->next_list->prev_list = obj->prev_list;
				if (obj->threadList->head == obj)
					obj->threadList->head = obj->next_list;
				if (obj->threadList->tail == obj)
					obj->threadList->tail = obj->prev_list;
				obj->threadList->size--;
				startTimer(val);

				delete obj;

				return true;
			}

			obj = nullptr;

			return true;
		}

		bool ExitThreadImpl(void* _exitCode, void*)
		{
			uintptr_t exitCode = (uintptr_t)_exitCode;
			g_currentThread->exitCode = exitCode;
			g_currentThread->status = THREAD_STATUS_DEAD;
			if (g_currentThread->prev_run)
				g_currentThread->prev_run->next_run = g_currentThread->next_run;
			if (g_currentThread->next_run)
				g_currentThread->next_run->prev_run = g_currentThread->prev_run;
			if (g_currentThread->priorityList->head == g_currentThread)
				g_currentThread->priorityList->head = g_currentThread->next_run;
			if (g_currentThread->priorityList->tail == g_currentThread)
				g_currentThread->priorityList->tail = g_currentThread->prev_run;
			g_currentThread->priorityList->size--;
			freeThreadStackInfo((void*)&g_currentThread->stackInfo);
			if(!g_currentThread->references)
				delete g_currentThread;
			startTimer(0);
			callScheduler();
			return false;
		}

		[[noreturn]] void ExitThread(uint32_t exitCode)
		{
			stopTimer();
			callBlockCallbackOnThread((taskSwitchInfo*)&g_currentThread->context, ExitThreadImpl, (void*)(uintptr_t)exitCode, nullptr);
			while (1);
		}
}
}