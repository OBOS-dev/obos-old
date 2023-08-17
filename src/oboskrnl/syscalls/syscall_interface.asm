; driver_api/syscall_interface.asm
;
; Copyright (c) 2023 Omar Berrow

%macro SYSCALL_DEFINE 2
[global %1]
%1:
%assign current_syscall current_syscall + 1
	mov eax, %2
	lea ebx, [esp+4]
	int 0x40
	ret
%endmacro

%assign current_syscall 0

SYSCALL_DEFINE ConsoleOutputString, current_syscall
SYSCALL_DEFINE ExitProcess, current_syscall
SYSCALL_DEFINE CreateProcess, current_syscall
SYSCALL_DEFINE TerminateProcess, current_syscall
SYSCALL_DEFINE OpenCurrentProcessHandle, current_syscall
SYSCALL_DEFINE OpenProcessHandle, current_syscall
SYSCALL_DEFINE CreateThread, current_syscall
SYSCALL_DEFINE OpenThread, current_syscall
SYSCALL_DEFINE PauseThread, current_syscall
SYSCALL_DEFINE ResumeThread, current_syscall
SYSCALL_DEFINE SetThreadPriority, current_syscall
SYSCALL_DEFINE SetThreadStatus, current_syscall
SYSCALL_DEFINE TerminateThread, current_syscall
SYSCALL_DEFINE GetTid, current_syscall
SYSCALL_DEFINE GetExitCode, current_syscall
SYSCALL_DEFINE GetThreadPriority, current_syscall
SYSCALL_DEFINE GetThreadStatus, current_syscall
SYSCALL_DEFINE WaitForThreadExit, current_syscall
SYSCALL_DEFINE ExitThread, current_syscall
SYSCALL_DEFINE GetCurrentThreadTid, current_syscall
SYSCALL_DEFINE GetCurrentThreadHandle, current_syscall
SYSCALL_DEFINE GetCurrentThreadPriority, current_syscall
SYSCALL_DEFINE GetCurrentThreadStatus, current_syscall
SYSCALL_DEFINE CloseHandle, current_syscall
SYSCALL_DEFINE Printf, current_syscall