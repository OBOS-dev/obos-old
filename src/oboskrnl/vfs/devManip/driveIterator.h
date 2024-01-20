/*
	oboskrnl/vfs/devManip/driveIterator.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

namespace obos
{
	namespace vfs
	{
		class DriveIterator final
		{
		public:
			OBOS_EXPORT DriveIterator();

			// These functions return the path of the next/current element. The lifetime of the path is controlled by the user.
			// Allocated with the kernel's new.

			OBOS_EXPORT const char* operator*();
			OBOS_EXPORT const char* operator++();
			OBOS_EXPORT const char* operator++(int);
			OBOS_EXPORT const char* operator--();
			OBOS_EXPORT const char* operator--(int);

			OBOS_EXPORT operator bool() { return m_currentNode; }

			~DriveIterator() {}
		private:
			void* m_currentNode = nullptr;
		};
	}
}