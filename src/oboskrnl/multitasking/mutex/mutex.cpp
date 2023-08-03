/*
	mutex.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <multitasking/mutex/mutex.h>
#include <multitasking/multitasking.h>

#include <inline-asm.h>

namespace obos
{
	namespace multitasking
	{
		bool MutexLockCallback(Thread* _this, PVOID _mutex)
		{
			Mutex* mutex = (Mutex*)_mutex;
			return !mutex->m_locked || mutex->m_resume;
		}

		void Mutex::Lock(bool waitIfLocked)
		{
			if (m_locked && waitIfLocked)
				return;
			EnterKernelSection();
			g_currentThread->isBlockedUserdata = this;
			g_currentThread->isBlockedCallback = MutexLockCallback;
			g_currentThread->status |= (UINT32_T)Thread::status_t::BLOCKED;
			LeaveKernelSection();
			_int(0x30);
			m_locked = true;
			m_lockOwner = g_currentThread->tid;
		}
		void Mutex::Unlock()
		{
			if (g_currentThread->tid != m_lockOwner)
				return;
			m_lockOwner = (DWORD)-1;
			m_locked = false;
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