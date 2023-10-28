/*
	driverInterface/x86_64/call.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <driverInterface/call.h>

//#include <arch/x86_64/interrupt.h>

namespace obos
{
	namespace driverInterface
	{
		uintptr_t g_syscallTable[256];

		uintptr_t RegisterSyscall(byte n, uintptr_t addr)
		{
			uintptr_t ret = g_syscallTable[n];
			g_syscallTable[n] = addr;
			return ret;
		}
	}
}