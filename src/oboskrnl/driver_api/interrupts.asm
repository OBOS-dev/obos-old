; driver_api/interrupt.asm
;
; Copyright (c) 2023 Omar Berrow

; extern "C" void isr80();
[global isr80]

segment .text

extern _ZN4obos9driverAPI14g_syscallTableE

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
	call [eax]

	iret