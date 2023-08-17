/*
	driver_api/syscall_interface.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

	void ConsoleOutputString(CSTRING string);
	void ExitProcess();
	DWORD CreateProcess(PBYTE elfFile, SIZE_T size, HANDLE* procHandle, HANDLE* mainThread);
	DWORD TerminateProcess(HANDLE proc);
	DWORD OpenCurrentProcessHandle(HANDLE* procHandle);
	DWORD OpenProcessHandle(HANDLE* procHandle, DWORD pid);
	DWORD CreateThread(HANDLE* thrHandle,
		DWORD threadPriority,
		VOID(*entry)(PVOID userData),
		PVOID userData,
		DWORD threadStatus,
		SIZE_T stackSizePages);
	DWORD OpenThread(HANDLE* thrHandle, DWORD tid);
	DWORD PauseThread(HANDLE thrHandle, BOOL force);
	DWORD ResumeThread(HANDLE thrHandle);
	DWORD SetThreadPriority(HANDLE thrHandle, DWORD newPriority);
	DWORD SetThreadStatus(HANDLE thrHandle, DWORD newStatus);
	DWORD TerminateThread(HANDLE thrHandle, DWORD exitCode);
	DWORD GetTid(HANDLE thrHandle, DWORD* tid);
	DWORD GetExitCode(HANDLE thrHandle, DWORD* exitCode);
	DWORD GetThreadPriority(HANDLE thrHandle, DWORD* priority);
	DWORD GetThreadStatus(HANDLE thrHandle, DWORD* status);
	DWORD WaitForThreadExit(HANDLE thrHandle);
	void ExitThread(DWORD exitCode);
	// Returns the tid of the current thread.
	DWORD GetCurrentThreadTid();
	// Returns a new handle of the current thread.
	HANDLE GetCurrentThreadHandle();
	// Returns the priority of the current thread.
	DWORD GetCurrentThreadPriority();
	// Returns the status of the current thread (most likely THREAD_RUNNING).
	DWORD GetCurrentThreadStatus();
	DWORD CloseHandle(HANDLE handle);
	void Printf(CSTRING format, ...);

#ifdef __cplusplus
}
#endif