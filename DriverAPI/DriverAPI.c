/*
	DriverAPI.c

	Copyright (c) 2023 Omar Berrow
*/

#define OBOS_DRIVER_ID
#include <DriverAPI/DriverAPI.h>

DWORD _CallSyscall(DWORD driverId, DWORD syscallId, PVOID arguments, BOOL isDriverSyscall)
{
	if (!isDriverSyscall)
	{
		DWORD exitCode = 0;
		asm volatile(
			"mov %2, %%ebx\n"
			"mov %1, %%eax\n"
			"int $0x40\n"
			"mov %%eax, %0"
			: "=r"(exitCode)
			: "r"(syscallId), "r"(arguments)
		);
		return exitCode;
	}
	else
	{
		DWORD exitCode = 0;
		asm volatile(
			"mov %3, %%ecx\n"
			"mov %2, %%ebx\n"
			"mov %1, %%eax\n"
			"int $0x50\n"
			"mov %%eax, %0"
			: "=r"(exitCode)
			: "r"(syscallId), "r"(arguments), "r"(driverId)
			);
		return exitCode;
	}
	return 0;
}

DWORD _RegisterDriver(DWORD driverId)
{
	DWORD(*arguments[1])();
	arguments[0] = OBOS_DriverDetach;
	DWORD err = _CallSyscall(driverId, 0, (PVOID)arguments, TRUE);
	if (err == OBOS_DRIVER_SYSCALL_ERROR_ALREADY_REGISTERED)
		while (1)
			// Yield execution.
			_CallSyscall(driverId, 1, NULLPTR, FALSE);
	return 0;
}
