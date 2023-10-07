; driver_api/syscall_interface.asm
;
; Copyright (c) 2023 Omar Berrow

%macro SYSCALL_DEFINE 2
[global %1]
%1:
%assign current_syscall current_syscall + 1
%ifdef __i686__
	mov eax, %2
	mov edi, esp
	add edi, 4
%elifdef __x86_64__
	mov rax, %2
	mov rdi, rsp
	add rdi, 8
%endif
	int 0x50
	ret
%endmacro

%assign current_syscall 1

SYSCALL_DEFINE RegisterInterruptHandler, current_syscall
SYSCALL_DEFINE PicSendEoi, current_syscall
SYSCALL_DEFINE EnableIRQ, current_syscall
SYSCALL_DEFINE DisableIRQ, current_syscall
SYSCALL_DEFINE PrintChar, current_syscall
SYSCALL_DEFINE GetMultibootModule, current_syscall
SYSCALL_DEFINE MapPhysicalTo, current_syscall
SYSCALL_DEFINE UnmapPhysicalTo, current_syscall
SYSCALL_DEFINE Printf, current_syscall
SYSCALL_DEFINE GetPhysicalAddress, current_syscall
SYSCALL_DEFINE ListenForConnections, current_syscall
SYSCALL_DEFINE ConnectionSendData, current_syscall
SYSCALL_DEFINE ConnectionRecvData, current_syscall
SYSCALL_DEFINE ConnectionClose, current_syscall

; All syscalls must be before this.

%ifdef __i686__
SYSCALL_DEFINE CallSyscall, [esp+8]
%endif
%ifdef __x86_64__
SYSCALL_DEFINE CallSyscall, [rsp+8]
%endif
