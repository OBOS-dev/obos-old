/*
	driver_api/interrupts.cpp

	Copyright (c) 2023 Omar Berrow.
*/

#include <driver_api/interrupts.h>

#include <utils/memory.h>

static DWORD do_nothing()
{ return -1; }

namespace obos
{
	namespace driverAPI
	{
		UINTPTR_T g_syscallTable[256];
		void ResetSyscallHandlers()
		{
			utils::dwMemset(g_syscallTable, (DWORD)do_nothing, 256);
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