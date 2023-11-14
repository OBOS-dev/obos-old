/*
	multitasking/locks/mutex.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
#ifndef MULTIASKING_THREAD_H_INCLUDED
	namespace thread
	{
		struct Thread;
	}
#endif
	namespace locks
	{
		class Mutex final
		{
		public:
			Mutex() = default;

			/// <summary>
			/// Locks the mutex.
			/// </summary>
			/// <param name="timeout">How long to block for, only effective if blockWithScheduler == true.</param>
			/// <param name="block">Whether to block if the lock is locked, or to abort.</param>
			/// <returns>Whether the mutex could be locked, otherwise false. If the function returned false, use GetLastError.</returns>
			bool Lock(uint64_t timeout = 0, bool block = true);
			/// <summary>
			/// Unlocks the mutex.
			/// </summary>
			/// <returns>Whether the mutex could be unlocked, otherwise false. If the function returned false, use GetLastError.</returns>
			bool Unlock();

			/// <summary>
			/// Returns whether the mutex is locked.
			/// </summary>
			/// <returns>Whether the mutex is locked..</returns>
			bool Locked() { return m_locked; }
			
			~Mutex();

		private:
			static bool LockBlockCallback(thread::Thread* thread, void* data);
			bool m_wake;
			bool m_locked;
			thread::Thread* m_ownerThread;
			struct LockQueueNode
			{
				LockQueueNode *next, *prev;
				thread::Thread* thread;
			};
			struct LockQueue
			{
				LockQueueNode *head, *tail;
				size_t size;
			};
			LockQueue m_queue;
		};
	}
}