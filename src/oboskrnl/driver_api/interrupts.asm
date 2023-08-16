; driver_api/interrupt.asm
;
; Copyright (c) 2023 Omar Berrow

; extern "C" void isr80();
[global isr80]

segment .text

extern _ZN4obos9driverAPI14g_syscallTableE
extern _ZN4obos18EnterKernelSectionEv
extern _ZN4obos18LeaveKernelSectionEv

; Dispatch driver syscalls.
; The syscall id is in eax.
; Arguments are passed on the stack backwards.
; The return value is in eax.
; Registers are NOT preserved.
isr80:
	cli

	; Get the syscall id.
	lea eax, [eax*4]
	lea eax, [_ZN4obos9driverAPI14g_syscallTableE+eax]

	push eax
	call _ZN4obos18EnterKernelSectionEv
	pop eax

	call [eax]

	push eax
	call _ZN4obos18LeaveKernelSectionEv
	pop eax

	iret