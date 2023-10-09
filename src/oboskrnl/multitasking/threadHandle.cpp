/*
	threadHandle.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <multitasking/threadHandle.h>
#include <multitasking/thread.h>
#include <multitasking/multitasking.h>

#include <memory_manager/paging/allocate.h>
#include <memory_manager/liballoc/liballoc.h>

#include <handle.h>

#include <inline-asm.h>
#include <types.h>
#include <error.h>

#include <process/process.h>

#include <utils/bitfields.h>
#include <external/list/list.h>

extern "C" [[noreturn]] void callExitThreadImpl(PBYTE rsp, DWORD par1, VOID(*func)(DWORD exitCode));

namespace obos
{
	namespace multitasking
	{
		ThreadHandle::ThreadHandle()
		{
			m_origin = this;
			m_references = 1;
			m_value = m_thread = nullptr;
		}


		DWORD ThreadHandle::CreateThread(Thread::priority_t threadPriority, VOID(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus, SIZE_T stackSizePages)
		{
			m_value = m_thread = new Thread();
			m_thread->nHandles++;
			return m_thread->CreateThread(threadPriority, entry, userData, threadStatus, stackSizePages);
		}
		bool ThreadHandle::OpenThread(Thread::Tid tid)
		{
			if (m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			list_iterator_t* iter = list_iterator_new(g_threads, LIST_HEAD);
			list_node_t* node = list_iterator_next(iter);
			for (; node != nullptr; node = list_iterator_next(iter))
				if (((Thread*)node->val)->tid == tid)
					break;
			if (!node)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return false;
			}
			bool ret = OpenThread((Thread*)node->val);
			return ret;
		}
		bool ThreadHandle::OpenThread(Thread* thread)
		{
			if (m_thread)
			{
				SetLastError(OBOS_ERROR_HANDLE_ALREADY_OPENED);
				return false;
			}
			if (thread->status == (UINT32_T)status_t::DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}
			m_value = m_thread = thread;
			m_references = 1;
			m_origin = this;
			thread->nHandles++;
			return true;
		}
		bool ThreadHandle::PauseThread(bool force)
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			if (m_thread->status == (UINT32_T)status_t::DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}
			// Pausing the current thread isn't a good idea...
			if (m_thread == g_currentThread && !force)
			{
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return false;
			}
			m_thread->status |= (UINT32_T)status_t::PAUSED;
			if (m_thread == g_currentThread)
				_int(0x30);
			return true;
		}
		bool ThreadHandle::ResumeThread()
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			if (m_thread->status == (UINT32_T)status_t::DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}
			m_thread->status &= ~((UINT32_T)status_t::PAUSED);
			return true;
		}
		bool ThreadHandle::SetThreadPriority(Thread::priority_t newPriority)
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			if (m_thread->status == (UINT32_T)status_t::DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}
			if (m_thread->priority == newPriority)
				return true;
			EnterKernelSection();
			list_t* priorityList = nullptr;
			list_t* oldPriorityList = nullptr;

			switch (newPriority)
			{
			case obos::multitasking::Thread::priority_t::IDLE:
				priorityList = g_threadPriorityList[0];
				break;
			case obos::multitasking::Thread::priority_t::LOW:
				priorityList = g_threadPriorityList[1];
				break;
			case obos::multitasking::Thread::priority_t::NORMAL:
				priorityList = g_threadPriorityList[2];
				break;
			case obos::multitasking::Thread::priority_t::HIGH:
				priorityList = g_threadPriorityList[3];
				break;
			default:
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				LeaveKernelSection();
				return false;
			}
			switch (m_thread->priority)
			{
			case obos::multitasking::Thread::priority_t::IDLE:
				oldPriorityList = g_threadPriorityList[0];
				break;
			case obos::multitasking::Thread::priority_t::LOW:
				oldPriorityList = g_threadPriorityList[1];
				break;
			case obos::multitasking::Thread::priority_t::NORMAL:
				oldPriorityList = g_threadPriorityList[2];
				break;
			case obos::multitasking::Thread::priority_t::HIGH:
				oldPriorityList = g_threadPriorityList[3];
				break;
			default:
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				LeaveKernelSection();
				return false;
			}

			m_thread->priority = newPriority;
			m_thread->iterations = 0;
			list_remove(oldPriorityList, list_find(oldPriorityList, m_value));
			list_rpush(priorityList, list_node_new(m_value));
			LeaveKernelSection();
			return true;
		}
		bool ThreadHandle::SetThreadStatus(utils::RawBitfield newStatus)
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			// Don't kill threads this way please.
			if (m_thread->status == (UINT32_T)status_t::DEAD || newStatus == (UINT32_T)status_t::DEAD || utils::testBitInBitfield(newStatus, 2))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (newStatus == m_thread->status)
				return true;
			newStatus &= ~((UINT32_T)status_t::DEAD);
			m_thread->status = newStatus;
			if (m_thread == g_currentThread)
				_int(0x30);
			return true;
		}

		bool ThreadHandle::TerminateThread(DWORD exitCode)
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			if (m_thread->status == (UINT32_T)status_t::DEAD)
			{
				SetLastError(OBOS_ERROR_THREAD_DIED);
				return false;
			}
			if (m_thread == g_currentThread)
			{
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return false; // Use ExitThread
			}
			// Kill the thread.
			m_thread->status = (UINT32_T)status_t::DEAD;
			m_thread->exitCode = exitCode;
			list_remove(m_thread->owner->threads, list_find(m_thread->owner->threads, m_thread));
			if (m_thread->owner->threads->len == 0)
				m_thread->owner->TerminateProcess(exitCode);
			memory::VirtualFree(m_thread->stackBottom, m_thread->stackSizePages);
			if(m_thread->tssStackBottom)
				memory::VirtualFree(m_thread->tssStackBottom, 2);
			return true;
		}

		Thread::Tid ThreadHandle::GetTid()
		{
			if (!m_thread)
				return -1;
			return m_thread->tid;
		}

		DWORD ThreadHandle::GetExitCode()
		{
			if (!m_thread)
				return 0;
			if (m_thread->tid == (DWORD)-1)
				return 0;
			if (m_thread->status != (UINT32_T)status_t::DEAD)
				return 0;
			return m_thread->exitCode;
		}

		Thread::priority_t ThreadHandle::GetThreadPriority()
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return Thread::priority_t::INVALID_PRIORITY;
			}
			return m_thread->priority;
		}

		utils::RawBitfield ThreadHandle::GetThreadStatus()
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return 0;
			}
			return m_thread->status;
		}

		bool ThreadHandle::WaitForThreadExit()
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			if (m_thread->status == (UINT32_T)status_t::DEAD)
				return true; // The thread already exited...
			if (m_thread == g_currentThread)
			{
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return false;
			}
			multitasking::g_currentThread->isBlockedCallback = [](multitasking::Thread*, PVOID userdata)->bool {
				multitasking::Thread* thread = (multitasking::Thread*)userdata;
				return utils::testBitInBitfield(thread->status, 0);
				};
			multitasking::g_currentThread->isBlockedUserdata = m_thread;
			multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::BLOCKED;
			_int(0x30);
			return true;
		}

		bool ThreadHandle::WaitForThreadStatusChange(utils::RawBitfield newStatus)
		{
			if (!m_thread)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			if (m_thread->status == (UINT32_T)status_t::DEAD || m_thread->status == newStatus)
				return true;
			if (m_thread == g_currentThread)
			{
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return false;
			}
			volatile UINTPTR_T udata[2] = {};
			if (!newStatus)
			{
				multitasking::g_currentThread->isBlockedCallback = [](multitasking::Thread*, PVOID userdata)->bool {
					UINTPTR_T* udata = (UINTPTR_T*)userdata;
					return ((multitasking::Thread*)(udata[0]))->status != udata[1];
					};
				udata[0] = (UINTPTR_T)m_thread;
				udata[1] = (UINTPTR_T)m_thread->status;
			}
			else
			{
				multitasking::g_currentThread->isBlockedCallback = [](multitasking::Thread*, PVOID userdata)->bool {
					UINTPTR_T* udata = (UINTPTR_T*)userdata;
					multitasking::Thread* thread = (multitasking::Thread*)(udata[0]);
					return thread->status == udata[1];
					};
				udata[0] = (UINTPTR_T)m_thread; 
				udata[1] = (UINTPTR_T)newStatus;
			}
			multitasking::g_currentThread->isBlockedUserdata = (PVOID)udata;
			multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::BLOCKED;
			_int(0x30);
			return true;
		}

		Handle* ThreadHandle::duplicate()
		{
			if (!m_value)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return nullptr;
			}
			ThreadHandle* newHandle = new ThreadHandle;
			newHandle->OpenThread(m_thread);
			return newHandle;
		}
		int ThreadHandle::closeHandle()
		{
			if (!m_thread)
				return m_origin->getReferences();
			if (!(--m_thread->nHandles) && m_thread->status == status_t::DEAD)
			{
				// We can free the thread's information.
				
				delete m_thread;

				/*list_t* priorityList = nullptr;

				switch (m_thread->priority)
				{
				case obos::multitasking::Thread::priority_t::IDLE:
					priorityList = g_threadPriorityList[0];
					break;
				case obos::multitasking::Thread::priority_t::LOW:
					priorityList = g_threadPriorityList[1];
					break;
				case obos::multitasking::Thread::priority_t::NORMAL:
					priorityList = g_threadPriorityList[2];
					break;
				case obos::multitasking::Thread::priority_t::HIGH:
					priorityList = g_threadPriorityList[3];
					break;
				default:
					LeaveKernelSection();
					return 0;
				}
				list_remove(priorityList, list_find(priorityList, m_value));
				list_remove(g_threads, list_find(g_threads, m_value));*/
			}
			m_thread = nullptr;
			m_value = nullptr;
			return m_origin->getReferences();
		}

		void ExitThreadImpl(DWORD exitCode)
		{
			if(g_currentThread->stackBottom)
				memory::VirtualFree(g_currentThread->stackBottom, g_currentThread->stackSizePages);
			if(g_currentThread->tssStackBottom)
				memory::VirtualFree(g_currentThread->tssStackBottom, 2);
			g_currentThread->exitCode = exitCode;
			g_currentThread->status = (UINT32_T)Thread::status_t::DEAD;
			list_remove(g_currentThread->owner->threads, list_find(g_currentThread->owner->threads, g_currentThread));
			list_remove(g_threads, list_find(g_threads, g_currentThread));
			list_t* priorityList = nullptr;
			switch (g_currentThread->priority)
			{
			case obos::multitasking::Thread::priority_t::IDLE:
				priorityList = g_threadPriorityList[0];
				break;
			case obos::multitasking::Thread::priority_t::LOW:
				priorityList = g_threadPriorityList[1];
				break;
			case obos::multitasking::Thread::priority_t::NORMAL:
				priorityList = g_threadPriorityList[2];
				break;
			case obos::multitasking::Thread::priority_t::HIGH:
				priorityList = g_threadPriorityList[3];
				break;
			default:
				break;
			}
			list_remove(priorityList, list_find(priorityList, g_currentThread));

			if (g_currentThread->owner->threads->len == 0)
				g_currentThread->owner->TerminateProcess(exitCode, false);
			//delete g_currentThread;
			_int(0x30);
		}

		void ExitThread(DWORD exitCode)
		{
			static BYTE tempStack[8192];
			callExitThreadImpl(tempStack + sizeof(tempStack), exitCode, ExitThreadImpl);
		}
		DWORD GetCurrentThreadTid()
		{
			if (!g_currentThread)
				return 0xFFFFFFFF;
			return g_currentThread->tid;
		}
		Thread::priority_t GetCurrentThreadPriority()
		{
			return g_currentThread->priority;
		}
		utils::RawBitfield GetCurrentThreadStatus()
		{
			return g_currentThread->status;
		}
		SIZE_T MillisecondsToTicks(SIZE_T milliseconds)
		{
			return milliseconds;
		}
		void Sleep(SIZE_T milliseconds)
		{
			g_currentThread->isBlockedCallback = [](multitasking::Thread* _this, PVOID)->bool
				{
					return multitasking::g_timerTicks >= _this->wakeUpTime;
				};
			multitasking::g_currentThread->wakeUpTime = multitasking::g_timerTicks + MillisecondsToTicks(milliseconds);
			multitasking::g_currentThread->status |= (utils::RawBitfield)multitasking::Thread::status_t::BLOCKED;
			_int(0x30);
		}
		ThreadHandle* GetCurrentThreadHandle()
		{
			ThreadHandle* handle = new ThreadHandle{}; 
			handle->OpenThread(g_currentThread);
			return handle;
		}
	}
}
