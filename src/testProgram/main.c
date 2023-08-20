// testProgram/main.c
// 
// Copyright (c) 2023 Omar Berrow

#include <types.h>

#include <syscalls/syscall_interface.h>

static char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
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
	ConsoleOutputString("Hello from tid ");
	ConsoleOutputString(str);
	ConsoleOutputString("\r\n");
	ExitThread(tid);
}

void _start()
{
	SetConsoleColor(0xFFFFF700, 0x00000000);
	ConsoleOutputString("testProgram started!\r\n");
	HANDLE thr1 = 0;
	DWORD status = CreateThread(&thr1, 1, Thread, 0, 0, 0);
	if (status)
	{
		CloseHandle(thr1);
		return;
	}
	DWORD tid = GetCurrentThreadTid();

	char str[12] = {};
	itoa(tid, str, 10);
	ConsoleOutputString("Hello from tid ");
	ConsoleOutputString(str);
	ConsoleOutputString("\r\n");

	WaitForThreadExit(thr1);
	CloseHandle(thr1);
	ConsoleOutputString("Exiting testProgram.\r\n");
	SetConsoleColor(0xFFFFFFFF, 0x00000000);
	ExitProcess(0);
}