/*
	multitasking/locks/mutex.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

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
			OBOS_EXPORT Mutex() : m_canUseMultitasking{ true } {};
			OBOS_EXPORT Mutex(bool canUseMultitasking) : m_canUseMultitasking{ canUseMultitasking } {};

			/// <summary>
			/// Locks the mutex.
			/// </summary>
			/// <param name="timeout">How long to block for, only effective if blockWithScheduler == true.</param>
			/// <param name="block">Whether to block if the lock is locked, or to abort.</param>
			/// <returns>Whether the mutex could be locked, otherwise false. If the function returned false, use GetLastError.</returns>
			OBOS_EXPORT bool Lock(uint64_t timeout = 0, bool block = true);
			/// <summary>
			/// Unlocks the mutex.
			/// </summary>
			/// <returns>Whether the mutex could be unlocked, otherwise false. If the function returned false, use GetLastError.</returns>
			OBOS_EXPORT bool Unlock();

			/// <summary>
			/// Returns whether the mutex is locked.
			/// </summary>
			/// <returns>Whether the mutex is locked..</returns>
			OBOS_EXPORT bool Locked() const;

			OBOS_EXPORT bool IsInitialized() const { return m_initialized; }
			
			OBOS_EXPORT void CanUseMultitasking(bool val) { m_canUseMultitasking = val; };
			OBOS_EXPORT bool CanUseMultitasking() const { return m_canUseMultitasking; };

			OBOS_EXPORT ~Mutex();

		private:
			bool m_wake;
			bool m_locked;
			bool m_canUseMultitasking = true;
			bool m_initialized;
			thread::Thread* m_ownerThread;
		};
	}
}