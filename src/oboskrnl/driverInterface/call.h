/*
	driverInterface/call.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace driverInterface
	{
		extern uintptr_t g_syscallTable[256];
		uintptr_t RegisterSyscall(byte n, uintptr_t addr);

		size_t SyscallVPrintf(void* pars);
		void* SyscallMalloc(size_t* size);
		void SyscallFree(void** block);
		void* SyscallMapPhysToVirt(void** pars);
		void* SyscallGetInitrdLocation(size_t** oSize);
	}
}
