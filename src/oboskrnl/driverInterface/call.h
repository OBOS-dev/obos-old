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
	}
}
