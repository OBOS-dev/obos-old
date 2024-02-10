/*
	oboskrnl/process/x86_64/procInfo.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <arch/x86_64/syscall/handle.h>

#include <utils/hashmap.h>

#include <multitasking/locks/mutex.h>

namespace obos
{
	namespace process
	{
		struct procContextInfo
		{
			void* cr3;
			// Controls ownership of 'handleTable' and 'nextHandleValue'.
			locks::Mutex handleTableLock;
			utils::Hashmap<syscalls::user_handle, syscalls::handle> handleTable;
			syscalls::user_handle nextHandleValue;
			struct virtuallyMappedRegionNode
			{
				void* base;
				void* addr;
				void* dirent;
				size_t off, size;
				// Whether dirent is a DirectoryEntry* (true) or a PartitionEntry* (false).
				// The former option is used for memory mapped files.
				// The latter option is used for swap space.
				bool isDirent;
			};
			static bool vmapEquals(void* const &_left, void* const &_right)
			{
				uintptr_t left  = (uintptr_t)_left,
					      right = (uintptr_t)_right;
				return (left & ~0xfff) == (right & ~0xfff);
			}
			utils::Hashmap<void*, virtuallyMappedRegionNode, vmapEquals> memoryMappedFiles;
		};
	}
}