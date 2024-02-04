/*
	oboskrnl/vfs/fileManip/directoryIterator.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace vfs
	{
		class DirectoryIterator final
		{
		public:
			DirectoryIterator() = default;

			bool OpenAt(const char* directory);

			// These functions return the path of the next/current element. The lifetime of the path is controlled by the user.
			// Allocated with the kernel's new.

			const char* operator*();
			const char* operator++();
			const char* operator++(int);
			const char* operator--();
			const char* operator--(int);

			operator bool() { return m_currentNode; }

			bool Close();

			~DirectoryIterator() 
			{ 
				if (m_currentNode && m_directoryNode)
					Close();
			}

			// This can be a mount point, or a directory entry. Beware!
			void* GetDirectoryNode() const { return m_directoryNode; }
			// This should only ever be a directory entry.
			void* GetCurrentNode() const { return m_currentNode; }
		private:
			void* m_directoryNode = nullptr;
			void* m_currentNode = nullptr;
		};
	}
}