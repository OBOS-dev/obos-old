/*
	arch/x86_64/syscall/register.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <arch/x86_64/syscall/register.h>
#include <arch/x86_64/syscall/thread.h>
#include <arch/x86_64/syscall/file.h>
#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/console.h>
#include <arch/x86_64/syscall/vmm.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t g_syscallTable[g_syscallTableLimit + 1];
		void RegisterSyscalls()
		{
			for (uint16_t currentSyscall = 0; currentSyscall < 13; RegisterSyscall(currentSyscall++, (uintptr_t)ThreadSyscallHandler));
			for (uint16_t currentSyscall = 13; currentSyscall < 23; RegisterSyscall(currentSyscall++, (uintptr_t)FileHandleSyscallHandler));
			RegisterSyscall(23, (uintptr_t)ThreadSyscallHandler);
			RegisterSyscall(24, (uintptr_t)SyscallInvalidateHandle);
			for (uint16_t currentSyscall = 25; currentSyscall < 38; RegisterSyscall(currentSyscall++, (uintptr_t)ConsoleSyscallHandler));
			for (uint16_t currentSyscall = 39; currentSyscall < 45; RegisterSyscall(currentSyscall++, (uintptr_t)VMMSyscallHandler));
		}
		void RegisterSyscall(uint16_t n, uintptr_t func)
		{
			if (n > g_syscallTableLimit)
				logger::warning("Hit the syscall limit. Overflowed by %d.\n", n - g_syscallTableLimit);
			g_syscallTable[n] = func;
		}
	}
}