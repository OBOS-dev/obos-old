/*
	multitasking/locks/mutex.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <atomic.h>
#include <memory_manipulation.h>

#include <multitasking/scheduler.h>
#include <multitasking/cpu_local.h>

#include <multitasking/locks/mutex.h>

namespace obos
{
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
			if (!m_locked || !thread::g_initialized)
			{
				atomic_set(&m_locked);
				if (thread::g_initialized && m_canUseMultitasking)
					m_ownerThread = (thread::Thread*)thread::GetCurrentCpuLocalPtr()->currentThread;
				return true;
			}
			if (m_locked)
			{
				auto currentThread = thread::GetCurrentCpuLocalPtr()->currentThread;
				if (m_ownerThread == currentThread)
					return true;
				LockQueueNode* queue_node = new LockQueueNode;
				utils::memzero(queue_node, sizeof(queue_node));
				queue_node->thread = (thread::Thread*)currentThread;
				if (m_queue.tail)
					m_queue.tail->next = queue_node;
				if (!m_queue.head)
					m_queue.head = queue_node;
				m_queue.tail = queue_node;
				m_queue.size++;
				uintptr_t userdata[2] = { (uintptr_t)this, (uintptr_t)queue_node };
				if (m_canUseMultitasking)
				{
					currentThread->wakeUpTime = timeout ? thread::g_timerTicks + timeout : 0xffffffffffffffff;
					currentThread->blockCallback.callback = LockBlockCallback;
					currentThread->blockCallback.userdata = userdata;
					currentThread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_BLOCKED;
					thread::callScheduler();
				}
				else
					while (!LockBlockCallback((thread::Thread*)currentThread, userdata));
				if (m_locked || m_wake)
				{
					SetLastError(OBOS_ERROR_TIMEOUT);
					if (queue_node->next)
						queue_node->next->prev = queue_node->prev;
					if (queue_node->prev)
						queue_node->prev->next = queue_node->next;
					delete queue_node;
					atomic_dec(m_queue.size);
					return false;
				}
				if (queue_node->next)
					queue_node->next->prev = queue_node->prev;
				if (queue_node->prev)
					queue_node->prev->next = queue_node->next;
				delete queue_node;
				atomic_dec(m_queue.size);
			}
			atomic_set(&m_locked);
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
			thread::callScheduler();
			m_ownerThread = nullptr;
			atomic_clear(&m_locked);
		}
	}
}