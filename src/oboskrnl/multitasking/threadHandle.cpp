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

#include <process/process.h>

#include <utils/bitfields.h>
#include <external/list/list.h>

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
				return false;
			EnterKernelSection();
			list_iterator_t* iter = list_iterator_new(g_threads, LIST_HEAD);
			list_node_t* node = list_iterator_next(iter);
			for (; node != nullptr; node = list_iterator_next(iter))
				if (((Thread*)node->val)->tid == tid)
					break;
			if (!node)
			{
				LeaveKernelSection();
				return false;
			}
			bool ret = OpenThread((Thread*)node->val);
			LeaveKernelSection();
			return ret;
		}
		bool ThreadHandle::OpenThread(Thread* thread)
		{
			if (m_thread)
				return false;
			if (thread->status == (UINT32_T)status_t::DEAD)
				return false;
			EnterKernelSection();
			m_value = m_thread = thread;
			m_references = 1;
			m_origin = this;
			thread->nHandles++;
			LeaveKernelSection();
			return true;
		}
		void ThreadHandle::PauseThread(bool force)
		{
			if (!m_thread)
				return;
			if (m_thread->tid == (DWORD)-1)
				return;
			if (m_thread->status == (UINT32_T)status_t::DEAD)
				return;
			// Pausing the current thread isn't a good idea...
			if (m_thread == g_currentThread && !force)
				return;
			m_thread->status |= (UINT32_T)status_t::PAUSED;
			if (m_thread == g_currentThread)
				_int(0x30);
		}
		void ThreadHandle::ResumeThread()
		{
			if (!m_thread)
				return;
			if (m_thread->tid == (DWORD)-1)
				return;
			if (m_thread->status == (UINT32_T)status_t::DEAD)
				return;
			m_thread->status &= ~((UINT32_T)status_t::PAUSED);
		}
		void ThreadHandle::SetThreadPriority(Thread::priority_t newPriority)
		{
			if (!m_thread)
				return;
			if (m_thread->tid == (DWORD)-1)
				return;
			if (m_thread->status == (UINT32_T)status_t::DEAD)
				return;
			if (m_thread->priority == newPriority)
				return;
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
				LeaveKernelSection();
				return;
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
				LeaveKernelSection();
				return;
			}

			m_thread->priority = newPriority;
			m_thread->iterations = 0;
			list_remove(oldPriorityList, list_find(oldPriorityList, m_value));
			list_rpush(priorityList, list_node_new(m_value));
			LeaveKernelSection();
		}
		void ThreadHandle::SetThreadStatus(utils::RawBitfield newStatus)
		{
			if (!m_thread)
				return;
			if (m_thread->tid == (DWORD)-1)
				return;
			// Don't kill threads this way please.
			if (m_thread->status == (UINT32_T)status_t::DEAD || newStatus == (UINT32_T)status_t::DEAD || utils::testBitInBitfield(newStatus, 2))
				return;
			if (newStatus == m_thread->status)
				return;
			newStatus &= ~((UINT32_T)status_t::DEAD);
			m_thread->status = newStatus;
			if (m_thread == g_currentThread)
				_int(0x30);
		}

		void ThreadHandle::TerminateThread(DWORD exitCode)
		{
			if (!m_thread)
				return;
			if (m_thread->tid == (DWORD)-1)
				return;
			if (m_thread->status == (UINT32_T)status_t::DEAD)
				return;
			if (m_thread == g_currentThread)
				return;
			// Kill the thread.
			EnterKernelSection();
			m_thread->status = (UINT32_T)status_t::DEAD;
			m_thread->exitCode = exitCode;
			list_remove(m_thread->owner->threads, list_find(m_thread->owner->threads, m_thread));
			if (m_thread->owner->threads->len == 0)
				m_thread->owner->TerminateProcess(exitCode);
			memory::VirtualFree(m_thread->stackBottom, m_thread->stackSizePages);
			LeaveKernelSection();
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
				return (Thread::priority_t)0;
			if (m_thread->tid == (DWORD)-1)
				return (Thread::priority_t)0;
			return m_thread->priority;
		}

		utils::RawBitfield ThreadHandle::GetThreadStatus()
		{
			if (!m_thread)
				return 0;
			if (m_thread->tid == (DWORD)-1)
				return 0;
			return m_thread->status;
		}

		bool ThreadHandle::WaitForThreadExit()
		{
			if (!m_thread)
				return false;
			if (m_thread->tid == (DWORD)-1)
				return false;
			if (m_thread->status == (UINT32_T)status_t::DEAD)
				return true; // The thread already exited...
			if (m_thread == g_currentThread)
				return false;
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
				return false;
			if (m_thread->tid == (DWORD)-1)
				return false;
			if (m_thread->status == (UINT32_T)status_t::DEAD || m_thread->status == newStatus)
				return true;
			if (m_thread == g_currentThread)
				return false;
			EnterKernelSection();
			if (!newStatus)
			{
				multitasking::g_currentThread->isBlockedCallback = [](multitasking::Thread*, PVOID userdata)->bool {
					UINTPTR_T* udata = (UINTPTR_T*)userdata;
					return ((multitasking::Thread*)(udata[0]))->status != udata[1];
					};
				UINTPTR_T udata[2] = { (UINTPTR_T)m_thread, m_thread->status };
				multitasking::g_currentThread->isBlockedUserdata = udata;
			}
			else
			{
				multitasking::g_currentThread->isBlockedCallback = [](multitasking::Thread*, PVOID userdata)->bool {
					UINTPTR_T* udata = (UINTPTR_T*)userdata;
					multitasking::Thread* thread = (multitasking::Thread*)(udata[0]);
					return thread->status == udata[1];
					};
				volatile UINTPTR_T udata[2] = { (UINTPTR_T)m_thread, newStatus };
				multitasking::g_currentThread->isBlockedUserdata = (PVOID)udata;
			}
			multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::BLOCKED;
			LeaveKernelSection();
			_int(0x30);
			return true;
		}

		Handle* ThreadHandle::duplicate()
		{
			if (!m_value)
				return nullptr;
			EnterKernelSection();
			ThreadHandle* newHandle = (ThreadHandle*)kcalloc(1, sizeof(ThreadHandle));
			newHandle->m_origin = newHandle;
			newHandle->m_references++;
			newHandle->m_value = m_value;
			newHandle->m_thread = m_thread;
			m_thread->nHandles++;
			m_references++;
			LeaveKernelSection();
			return newHandle;
		}
		int ThreadHandle::closeHandle()
		{
			if (!m_thread)
				return m_origin->getReferences();
			if (!(--m_thread->nHandles) && m_thread->status == (UINT32_T)status_t::DEAD && !(--m_origin->getReferences()))
			{
				// We can free the thread's information.

				EnterKernelSection();
				list_t* priorityList = nullptr;

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
				list_remove(g_threads, list_find(g_threads, m_value));

				delete m_thread;
				LeaveKernelSection();
			}
			m_thread = nullptr;
			m_value = nullptr;
			return m_origin->getReferences();
		}

		void ExitThread(DWORD exitCode)
		{
			EnterKernelSection();
			ThreadHandle handle;
			handle.OpenThread(g_currentThread);
			handle.m_thread->exitCode = exitCode;
			handle.m_thread->status = (UINT32_T)ThreadHandle::status_t::DEAD;
			list_remove(g_currentThread->owner->threads, list_find(g_currentThread->owner->threads, g_currentThread));
			handle.closeHandle();
			LeaveKernelSection();
			_int(0x30);
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
		ThreadHandle* GetCurrentThreadHandle()
		{
			ThreadHandle* handle = new ThreadHandle{}; 
			handle->OpenThread(g_currentThread);
			return handle;
		}
	}
}
