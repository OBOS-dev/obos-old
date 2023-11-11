/*
	arch/x86_64/syscall/register.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <arch/x86_64/syscall/register.h>
#include <arch/x86_64/syscall/verify_pars.h>
#include <arch/x86_64/syscall/memory_syscalls.h>
#include <arch/x86_64/syscall/console_syscalls.h>
#include <arch/x86_64/syscall/thread_syscalls.h>

namespace obos
{
	namespace syscalls
	{
		uint32_t SyscallGetLastError()
		{
			return GetLastError();
		}
		void SyscallSetLastError(uint32_t* newError)
		{
			if(!canAccessUserMemory(newError, sizeof(*newError), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			SetLastError(*newError);
		}

		uintptr_t g_syscallTable[g_syscallTableLimit + 1];
		void RegisterSyscalls()
		{
			uint16_t currentSyscall = 0;
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallVirtualAlloc);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallVirtualFree);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallVirtualProtect);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallVirtualGetProtection);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallAllocConsole);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleOutputCharacter);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleOutputCharacterAt);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleOutputCharacterAtWithColour);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleOutputString);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleOutputStringSz);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleSetPosition);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleGetPosition);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleSetColour);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleGetColour);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleSetFont);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleGetFont);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleSetFramebuffer);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleGetFramebuffer);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallConsoleGetConsoleBounds);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallFreeConsole);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallMakeThreadObject);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallCreateThread);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallOpenThread);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallPauseThread);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallResumeThread);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallSetThreadPriority);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallTerminateThread);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallGetThreadStatus);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallGetThreadExitCode);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallGetThreadLastError);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallCloseThread);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallFreeThread);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallGetLastError);
			RegisterSyscall(currentSyscall++, (uintptr_t)SyscallSetLastError);
		}
		void RegisterSyscall(uint16_t n, uintptr_t func)
		{
			if (n > g_syscallTableLimit)
				logger::warning("Hit the syscall limit. Overflowed by %d.\n", n - g_syscallTableLimit);
			g_syscallTable[n] = func;
		}
	}
}