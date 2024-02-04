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
		};
	}
}