/*
	arch/x86_64/syscall/register.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace syscalls
	{
		// Zero-based.
		constexpr size_t g_syscallTableLimit = 0x1fff;
		extern uintptr_t g_syscallTable[g_syscallTableLimit + 1];
		void RegisterSyscalls();
		void RegisterSyscall(uint16_t n, uintptr_t func);
	}
}