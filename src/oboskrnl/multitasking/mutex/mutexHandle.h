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

			Handle* duplicate() override;
			int closeHandle() override;

			bool Lock(bool waitIfLocked = true);
			bool Unlock();
			bool IsLocked();

			~MutexHandle();
		};
	}
}