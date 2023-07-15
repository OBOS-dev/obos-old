/*
	syscall.c

	Copyright (c) 2023 Omar Berrow
*/

#include "types.h"
#include "error.h"
#include "terminal.h"
#include "multitasking.h"

/// <summary>
/// Syscall table:
///	SYSCALL_TERMINAL_OUTPUT - 0x00: void TerminalOutput(CSTRING data, SIZE_T size);
/// SYSCALL_YIELD			- 0x01: void YieldExecution();
/// SYSCALL_EXIT_THREAD		- 0x02: void ExiTThread(DWORD exitCode);
/// </summary>
enum
{
	SYSCALL_TERMINAL_OUTPUT,
	SYSCALL_YIELDEXECUTION,
	SYSCALL_EXIT_THREAD,
};

/// <summary>
/// Handles a syscall.
/// </summary>
/// <param name="id">An id of a syscall. See the syscall table above for information.</param>
/// <param name="arguments">A pointer to the arguments for the syscall.</param>
/// <returns>The exit code of the syscall.</returns>
UINTPTR_T syscall_handler(DWORD id, PVOID arguments)
{
	UINTPTR_T exitCode = 0;
	switch (id)
	{
		case SYSCALL_TERMINAL_OUTPUT:
		{
			CSTRING message = *(CSTRING*)arguments;
			INT size = *(((PINT)arguments) + 1);
			TerminalOutput(message, size);
			break;
		}
		case SYSCALL_YIELDEXECUTION:
		{
			YieldExecution();
			break;
		}
		case SYSCALL_EXIT_THREAD:
		{
			DWORD* exitCode = (DWORD*)arguments;
			ExitThread(*exitCode);
			break;
		}
	}
	return exitCode;
}