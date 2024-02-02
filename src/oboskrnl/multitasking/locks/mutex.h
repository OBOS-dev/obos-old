/*
	multitasking/locks/mutex.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

#include <allocators/slab.h>

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
			OBOS_EXPORT Mutex() : m_canUseMultitasking{ true }, m_initialized{ true } {};
			OBOS_EXPORT Mutex(bool canUseMultitasking) : m_canUseMultitasking{ canUseMultitasking }, m_initialized{ true } {};

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

			[[nodiscard]] void* operator new(size_t )
			{
				return ImplSlabAllocate(ObjectTypes::Mutex);
			}
			void operator delete(void* ptr)
			{
				ImplSlabFree(ObjectTypes::Mutex, ptr);
			}
			void* operator new[](size_t sz)
			{
				return ImplSlabAllocate(ObjectTypes::Mutex, sz / sizeof(Mutex));
			}
			void operator delete[](void* ptr, size_t sz)
			{
				ImplSlabFree(ObjectTypes::Mutex, ptr, sz / sizeof(Mutex));
			}
			[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
			[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
			void operator delete(void*, void*) noexcept {}
			void operator delete[](void*, void*) noexcept {}
		private:
			bool m_wake;
			bool m_locked;
			bool m_canUseMultitasking = true;
			bool m_initialized;
			thread::Thread* m_ownerThread;
		};

		struct SafeMutex
		{
		public:
			SafeMutex() = delete;
			SafeMutex(Mutex* obj)
				:m_obj{obj}
			{}
			SafeMutex(const SafeMutex&) = default;
			SafeMutex& operator=(const SafeMutex&) = default;
			SafeMutex(SafeMutex&&) = default;
			SafeMutex& operator=(SafeMutex&&) = default;

			bool Lock(uint64_t timeout = 0, bool block = true)
			{
				if (!m_obj)
					return false;
				return m_obj->Lock(timeout, block);
			}
			bool Unlock()
			{
				if (!m_obj)
					return false;
				return m_obj->Unlock();
			}

			bool Locked() const
			{
				if (!m_obj)
					return false;
				return m_obj->Locked();
			}

			Mutex* Get() const
			{
				return m_obj;
			}

			~SafeMutex()
			{
				Unlock();
			}
		private:
			Mutex* m_obj = nullptr;
		};
	}
}