/*
	multitasking/locks/mutex.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>
#include <atomic.h>

#include <multitasking/scheduler.h>
#include <multitasking/cpu_local.h>

#include <multitasking/locks/mutex.h>

#include <liballoc/liballoc.h>

extern obos::locks::Mutex g_allocatorMutex;

namespace obos
{
	namespace thread
	{
		extern locks::Mutex g_coreGlobalSchedulerLock;
	}
	namespace locks
	{
		bool Mutex::LockBlockCallback(thread::Thread* thread, void* data)
		{
			uintptr_t* _data = (uintptr_t*)data;
			Mutex* _this = (Mutex*)_data[0];
			LockQueueNode* queue_node = (LockQueueNode*)_data[1];
			return _this->m_wake || (!_this->m_locked && _this->m_queue.tail == queue_node) || thread::g_timerTicks >= thread->wakeUpTime;
		}
		bool Mutex::Lock(uint64_t timeout, bool block)
		{
			if (!block && Locked())
			{
				SetLastError(OBOS_ERROR_MUTEX_LOCKED);
				return false;
			}
			// Compare m_locked with zero, and if it is zero, then set it to true and return true, otherwise return false and keep m_locked intact.
			uint64_t wakeupTime = thread::g_timerTicks + timeout;
			if (timeout == 0)
				wakeupTime = UINT64_MAX;
			while ((atomic_cmpxchg(&m_locked, 0, true) || m_wake) && thread::g_timerTicks < wakeupTime);
			if (thread::g_initialized && m_canUseMultitasking)
				m_ownerThread = (thread::Thread*)thread::GetCurrentCpuLocalPtr()->currentThread;
			return true;
		}
		bool Mutex::Unlock()
		{
			if (thread::g_initialized && m_canUseMultitasking)
			{
				if (thread::GetCurrentCpuLocalPtr()->currentThread != m_ownerThread)
				{
					SetLastError(OBOS_ERROR_ACCESS_DENIED);
					return false;
				}
			}
			atomic_clear(&m_locked);
			m_ownerThread = nullptr;
			return true;

		}
		Mutex::~Mutex()
		{
			atomic_set(&m_wake);
			thread::callScheduler(false);
			m_ownerThread = nullptr;
			atomic_clear(&m_locked);
		}
	}
}