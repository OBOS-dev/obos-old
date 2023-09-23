// testProgram/main.c
// 
// Copyright (c) 2023 Omar Berrow

#include <types.h>

#include <syscalls/syscall_interface.h>

static char* itoa(int value, char* result, int base) {
	const char /*__attribute__((__section__(".data")))*/ digit_str[] = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	int tmp_value;


	do {
		tmp_value = value;
		value /= base;
		*ptr++ = digit_str[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}


void Thread(PVOID userdata)
{
	DWORD tid = GetCurrentThreadTid();
	char str[12] = {};
	itoa(tid, str, 10);
	ConsoleOutputString(PASS_OBOS_API_PARS "Hello from tid ");
	ConsoleOutputString(PASS_OBOS_API_PARS str);
	ConsoleOutputString(PASS_OBOS_API_PARS "\r\n");
	ConsoleOutputString(PASS_OBOS_API_PARS "Calculating 10 Fibonacci numbers.\n");
	int op1 = 0;
	int op2 = 1;
	int prev = 0;
	for (int i = 0; i < 10; i++)
	{
		prev = op1 + op2;
		op1 = op2;
		op2 = prev;

		itoa(i + 1, str, 10);
		ConsoleOutputString(PASS_OBOS_API_PARS str);
		ConsoleOutputString(PASS_OBOS_API_PARS ": ");
		itoa(prev, str, 10);
		ConsoleOutputString(PASS_OBOS_API_PARS str);
		ConsoleOutputString(PASS_OBOS_API_PARS "\r\n");
	}
	ConsoleSwapBuffers();
	ExitThread(PASS_OBOS_API_PARS tid);
}

DWORD _start()
{
	SetConsoleColor(PASS_OBOS_API_PARS 0xFFFFF700, 0x00000000);
	ConsoleOutputString(PASS_OBOS_API_PARS "testProgram started!\r\n");
	HANDLE thr1 = 0;
	DWORD status = CreateThread(PASS_OBOS_API_PARS &thr1, 4, Thread, 0, 0, 0);
	if (status)
	{
		if (thr1)
			CloseHandle(PASS_OBOS_API_PARS thr1);
		return status;
	}
	DWORD tid = GetCurrentThreadTid();

	char str[12] = {};
	itoa(tid, str, 10);
	ConsoleOutputString(PASS_OBOS_API_PARS "Hello from tid ");
	ConsoleOutputString(PASS_OBOS_API_PARS str);
	ConsoleOutputString(PASS_OBOS_API_PARS "\r\n");

	WaitForThreadExit(PASS_OBOS_API_PARS thr1);
	CloseHandle(PASS_OBOS_API_PARS thr1);
	ExitThread(PASS_OBOS_API_PARS 0);
	return 0;
}