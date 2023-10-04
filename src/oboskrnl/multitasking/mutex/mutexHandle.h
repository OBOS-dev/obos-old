/*
	mutexHandle.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma	once

#include <handle.h>

namespace obos
{
	namespace multitasking
	{
		class MutexHandle final : private Handle
		{
		public:
			handleType getType() override
			{
				return handleType::mutex;
			}

			MutexHandle();
			MutexHandle(bool avoidDeadLocks);

			Handle* duplicate() override;
			int closeHandle() override;

			bool Lock(bool waitIfLocked = true);
			bool Unlock();
			bool IsLocked();
			DWORD GetLockOwnerTid();

			~MutexHandle();
		private:
			void init(bool avoidDeadLocks = true);
		};
		struct safe_lock
		{
			safe_lock() = delete;
			safe_lock(MutexHandle* mutex)
			{
				m_mutex = mutex;
			}
			bool Lock(bool waitIfLocked = true)
			{
				if (m_mutex)
				{
					m_mutex->Lock(waitIfLocked);
					return m_mutex->IsLocked();
				}
				return false;
			}
			bool IsLocked()
			{
				if (m_mutex)
					return m_mutex->IsLocked();
				return false;
			}
			void Unlock()
			{
				if (m_mutex)
					m_mutex->Unlock();
			}
			~safe_lock()
			{
				Unlock();
			}
		private:
			obos::multitasking::MutexHandle* m_mutex = nullptr;
		};
	}
}