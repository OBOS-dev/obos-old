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
			constexpr static handleType getType()
			{
				return handleType::mutex;
			}

			MutexHandle();

			Handle* duplicate() override;
			int closeHandle() override;

			void Lock(bool waitIfLocked = true);
			void Unlock();

			~MutexHandle();
		};
	}
}