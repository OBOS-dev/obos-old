; oboskrnl/syscalls/interrupt.asm
;
; Copyright (c) 2023 Omar Berrow
[BITS 32]


; extern "C" void isr64();
[global isr64]

extern _ZN4obos7process14g_syscallTableE

segment .data
ret: dd 0

extern _ZN4obos7process14g_syscallTableE

segment .text
extern _ZN4obos12EnterSyscallEv
extern _ZN4obos11ExitSyscallEv

; Dispatch program syscalls.
; The syscall id is in eax.
; A pointer to the parameters in reverse in ebx.
; The return value is in eax.
; Registers are preserved, except for eax.
isr64:
	cli
	pushad
	
	; Get the syscall id.
	lea eax, [eax*4]
	lea eax, [_ZN4obos7process14g_syscallTableE+eax]

	pushad
	call _ZN4obos12EnterSyscallEv
	popad

	push ebx
	call [eax]
	pop ebx

	pushad
	call _ZN4obos11ExitSyscallEv
	popad

	mov [ret], eax

	popad

	mov eax, [ret]

	iret