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

#include <allocators/liballoc.h>

extern obos::locks::Mutex g_allocatorMutex;

#ifdef __INTELLISENSE__
template<class type>
bool __atomic_compare_exchange_n(type* ptr, type* expected, type desired, bool weak, int success_memorder, int failure_memorder);
#endif

namespace obos
{
	namespace thread
	{
		extern locks::Mutex g_coreGlobalSchedulerLock;
	}
	namespace locks
	{
		bool Mutex::Lock(uint64_t timeout, bool block)
		{
			if (!block && Locked())
			{
				if (this != &thread::g_coreGlobalSchedulerLock)
					SetLastError(OBOS_ERROR_MUTEX_LOCKED);
				return false;
			}
			// Compare m_locked with zero, and if it is zero, then set it to true and return true, otherwise return false and keep m_locked intact.
			uint64_t wakeupTime = thread::g_timerTicks + timeout;
			if (timeout == 0)
				wakeupTime = 0xffffffffffffffff /* UINT64_MAX */;
			const bool expected = false;
			if (m_locked)
			{
				if (m_canUseMultitasking && thread::g_initialized)
				{
					thread::Thread* currentThread = (thread::Thread*)thread::GetCurrentCpuLocalPtr()->currentThread;
					currentThread->blockCallback.callback = [](thread::Thread* thr, void* udata)->bool
					{
						Mutex* _this = (Mutex*)udata;
						return (_this->m_locked && !_this->m_wake && thread::g_timerTicks < thr->wakeUpTime);
					};
					currentThread->blockCallback.userdata = this;
					currentThread->wakeUpTime = wakeupTime;
					currentThread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_BLOCKED;
					thread::callScheduler(false);
				}
				else
				{
					while (m_locked && !m_wake && thread::g_timerTicks < wakeupTime);
				}
			}
			while ((__atomic_compare_exchange_n(&m_locked, (bool*)&expected, true, false, 0, 0) || m_wake) || thread::g_timerTicks >= wakeupTime);
			if (thread::g_timerTicks >= wakeupTime)
			{
				SetLastError(OBOS_ERROR_TIMEOUT);
				return false;
			}
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
		bool Mutex::Locked() const
		{
			return atomic_test(&m_locked);
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