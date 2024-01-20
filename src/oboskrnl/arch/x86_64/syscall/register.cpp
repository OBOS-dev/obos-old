/*
	arch/x86_64/syscall/register.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <arch/x86_64/syscall/register.h>
#include <arch/x86_64/syscall/thread.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t g_syscallTable[g_syscallTableLimit + 1];
		void RegisterSyscalls()
		{
			uint16_t currentSyscall = 0;
			for (; currentSyscall < 13; RegisterSyscall(currentSyscall++, (uintptr_t)ThreadSyscallHandler));
		}
		void RegisterSyscall(uint16_t n, uintptr_t func)
		{
			if (n > g_syscallTableLimit)
				logger::warning("Hit the syscall limit. Overflowed by %d.\n", n - g_syscallTableLimit);
			g_syscallTable[n] = func;
		}
	}
}