/*
	arch/x86_64/syscall/verify_pars.h

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <arch/x86_64/syscall/verify_pars.h>

#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <multitasking/scheduler.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace syscalls
	{
		bool canAccessUserMemory(void* addr, size_t size, bool hasToWrite, bool checkingHandle)
		{
			if (checkingHandle && (uintptr_t)addr < 0xfffffffff0000070)
				return false;
			bool checkUsermode = checkingHandle && ((process::Process*)thread::g_currentThread->owner)->isUsermode;
			size_t nPagesToCheck = ((size + 0xfff) & ~0xfff) / 4096;
			uintptr_t* pageFlags = new uintptr_t[nPagesToCheck];
			uintptr_t requiredFlags = memory::PROT_IS_PRESENT | ((uintptr_t)checkUsermode * memory::PROT_USER_MODE_ACCESS);
			if(!memory::VirtualGetProtection((void*)((uintptr_t)addr & ~0xfff), nPagesToCheck, pageFlags))
				return false;
			for (size_t i = 0; i < nPagesToCheck; i++)
			{
				if (((pageFlags[i] & requiredFlags) != requiredFlags) || ((pageFlags[0] & memory::PROT_READ_ONLY) && hasToWrite))
				{
					delete[] pageFlags;
					return false;
				}
			}
			delete[] pageFlags;
			return true;
		}
	}
}