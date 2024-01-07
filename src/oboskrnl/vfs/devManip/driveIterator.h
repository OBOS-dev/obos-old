/*
	oboskrnl/vfs/devManip/driveIterator.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace vfs
	{
		class DriveIterator final
		{
		public:
			DriveIterator();

			// These functions return the path of the next/current element. The lifetime of the path is controlled by the user.
			// Allocated with the kernel's new.

			const char* operator*();
			const char* operator++();
			const char* operator++(int);
			const char* operator--();
			const char* operator--(int);

			operator bool() { return m_currentNode; }

			~DriveIterator() {}
		private:
			void* m_currentNode = nullptr;
		};
	}
}