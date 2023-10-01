/*
	mutex.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <inline-asm.h>
#include <error.h>

#include <multitasking/multitasking.h>
#include <multitasking/mutex/mutex.h>

namespace obos
{
	namespace multitasking
	{
		bool MutexLockCallback(Thread*, PVOID _mutex)
		{
			Mutex* mutex = (Mutex*)_mutex;
			return !mutex->m_locked || mutex->m_resume;
		}

		bool Mutex::Lock(bool waitIfLocked)
		{
			if (!g_initialized)
				return true;
			if (m_locked && !waitIfLocked)
			{
				SetLastError(OBOS_ERROR_AVOIDED_DEADLOCK);
				return false;
			}
			if (m_locked && m_lockOwner == g_currentThread->tid)
			{
				SetLastError(OBOS_ERROR_AVOIDED_DEADLOCK);
				return true;
			}
			if (m_locked)
			{
				EnterKernelSection();
				g_currentThread->isBlockedUserdata = this;
				g_currentThread->isBlockedCallback = MutexLockCallback;
				g_currentThread->status |= (UINT32_T)Thread::status_t::BLOCKED;
				LeaveKernelSection();
				_int(0x30);
			}
			m_locked = true;
			m_lockOwner = g_currentThread->tid;
			return true;
		}
		bool Mutex::Unlock()
		{
			DWORD lastError = GetLastError();
			SetLastError(OBOS_ERROR_ACCESS_DENIED);
			if(g_currentThread)
				if (g_currentThread->tid != m_lockOwner || !g_initialized)
					return false;
			m_lockOwner = (DWORD)-1;
			m_locked = false;
			SetLastError(lastError);
		}
		Mutex::~Mutex()
		{
			m_resume = true;
			Unlock();
			asm volatile("sti");
			hlt();
		}
	}
}