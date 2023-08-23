/*
	driver_api/interrupts.cpp

	Copyright (c) 2023 Omar Berrow.
*/

#include <driver_api/interrupts.h>

#include <utils/memory.h>

DWORD do_nothing()
{ return -1; }

namespace obos
{
	namespace driverAPI
	{
		UINTPTR_T g_syscallTable[256];
		void ResetSyscallHandlers()
		{
#ifdef __i686__
			utils::dwMemset(g_syscallTable, do_nothing, 256);
#else
			for (int i = 0; i < 256; i++)
				g_syscallTable[i] = (UINTPTR_T)do_nothing;
#endif

		}
		void RegisterSyscallHandler(BYTE syscall, UINTPTR_T address)
		{
			g_syscallTable[syscall] = address;
		}
		void UnregisterSyscallHandler(BYTE syscall)
		{
			g_syscallTable[syscall] = (UINTPTR_T)do_nothing;
		}
	}
}