/*
	driver_api/syscall_interface.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __x86_64__
#define OBOS_API UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,
#define PASS_OBOS_API_PARS 0,0,0,0,0,0,
#else
#define OBOS_API
#define PASS_OBOS_API_PARS
#endif

	void ConsoleOutputString(OBOS_API CSTRING string);
	void ExitProcess(OBOS_API DWORD exitCode);
	DWORD CreateProcess(OBOS_API PBYTE elfFile, SIZE_T size, HANDLE* procHandle, HANDLE* mainThread);
	DWORD TerminateProcess(OBOS_API HANDLE proc, DWORD exitCode);
	DWORD OpenCurrentProcessHandle(OBOS_API HANDLE* procHandle);
	DWORD OpenProcessHandle(OBOS_API HANDLE* procHandle, DWORD pid);
	DWORD CreateThread(OBOS_API HANDLE* thrHandle,
		DWORD threadPriority,
		VOID(*entry)(PVOID userData),
		PVOID userData,
		DWORD threadStatus,
		SIZE_T stackSizePages);
	DWORD OpenThread(OBOS_API HANDLE* thrHandle, DWORD tid);
	DWORD PauseThread(OBOS_API HANDLE thrHandle, BOOL force);
	DWORD ResumeThread(OBOS_API HANDLE thrHandle);
	DWORD SetThreadPriority(OBOS_API HANDLE thrHandle, DWORD newPriority);
	DWORD SetThreadStatus(OBOS_API HANDLE thrHandle, DWORD newStatus);
	DWORD TerminateThread(OBOS_API HANDLE thrHandle, DWORD exitCode);
	DWORD GetTid(OBOS_API HANDLE thrHandle, DWORD* tid);
	DWORD GetExitCode(OBOS_API HANDLE thrHandle, DWORD* exitCode);
	DWORD GetThreadPriority(OBOS_API HANDLE thrHandle, DWORD* priority);
	DWORD GetThreadStatus(OBOS_API HANDLE thrHandle, DWORD* status);
	DWORD WaitForThreadExit(OBOS_API HANDLE thrHandle);
	void ExitThread(OBOS_API DWORD exitCode);
	// Returns the tid of the current thread.
	DWORD GetCurrentThreadTid();
	// Returns a new handle of the current thread.
	HANDLE GetCurrentThreadHandle();
	// Returns the priority of the current thread.
	DWORD GetCurrentThreadPriority();
	// Returns the status of the current thread (most likely THREAD_RUNNING).
	DWORD GetCurrentThreadStatus();
	DWORD CloseHandle(OBOS_API HANDLE handle);
	DWORD OpenProcessParentHandle(OBOS_API HANDLE procHandle, HANDLE* parentHandle);
	DWORD SetConsoleColor(OBOS_API UINT32_T foreground, UINT32_T background);
	DWORD GetConsoleColor(OBOS_API UINT32_T* foreground, UINT32_T* background);
	DWORD ConsoleOutputCharacter(OBOS_API CHAR ch, BOOL swapBuffers);
	DWORD ConsoleOutputCharacterAt(OBOS_API CHAR ch, DWORD x, DWORD y, BOOL swapBuffers);
	DWORD ConsoleOutput(OBOS_API CSTRING message, BOOL swapBuffers);
	DWORD ConsoleFillLine(OBOS_API char ch, BOOL swapBuffers);
	DWORD SetConsoleCursorPosition(OBOS_API DWORD x, DWORD y);
	DWORD GetConsoleCursorPosition(OBOS_API DWORD* x, DWORD* y);
	DWORD ConsoleSwapBuffers();
	DWORD ClearConsole();
	PVOID VirtualAlloc(OBOS_API PVOID base, SIZE_T nPages, UINTPTR_T flags);
	DWORD VirtualFree(OBOS_API PVOID base, SIZE_T nPages);
	BOOL HasVirtualAddress(OBOS_API PCVOID base, SIZE_T nPages);
	DWORD MemoryProtect(OBOS_API PVOID base, SIZE_T nPages, UINTPTR_T flags);
	DWORD GetLastError();
	DWORD SetLastError(DWORD newLastError);

#ifdef __cplusplus
}
#endif