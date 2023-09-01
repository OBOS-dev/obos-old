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
; The syscall id is in e(r)ax.
; Arguments are passed on the stack backwards.
; The return value is in e(r)ax.
; Registers are NOT preserved.
isr80:
	cli

	; Get the syscall id.
%ifdef __i686__
%define accumulator eax
%define int_ret iret
%define	ptr_sz 4
%endif __i686__
%ifdef __x86_64__
%define accumulator rax
%define int_ret iretq
%define	ptr_sz 8
%endif
	lea accumulator, [accumulator*ptr_sz]
	lea accumulator, [_ZN4obos9driverAPI14g_syscallTableE+accumulator]

	push accumulator
	call _ZN4obos18EnterKernelSectionEv
	pop accumulator

	call [accumulator]

	push accumulator
	call _ZN4obos18LeaveKernelSectionEv
	pop accumulator

	int_ret