; oboskrnl/syscalls/interrupt.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 32]

; extern "C" void isr64();
[global isr64]

extern _ZN4obos7process14g_syscallTableE

segment .data
ret: dq 0

extern _ZN4obos7process14g_syscallTableE

segment .text
extern _ZN4obos12EnterSyscallEv
extern _ZN4obos11ExitSyscallEv

%ifdef __x86_64__
[BITS 64]
%macro pushaq 0
push rax
; rax has rsp.
mov rax, rsp
add rax, 8
push rcx
push rdx
push rbx
push rax ; Push rsp
push rbp
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
%endmacro

; Cleans up after pushaq.

%macro popaq 0
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rdi
pop rsi
pop rbp
add rsp, 8
pop rbx
pop rdx
pop rcx
pop rax
%endmacro

%define push_gpurpose pushaq
%define pop_gpurpose popaq
%define accumulator rax
%define pass_par1 mov rdi, rbx
%define clean_pass_par1
%define	ptr_sz 8

%elifdef

%define push_gpurpose pushad
%define pop_gpurpose popad
%define accumulator eax
%define pass_par1 push ebx
%define clean_pass_par1 add esp, 4
%define	ptr_sz 4

%endif

; Dispatch program syscalls.
; The syscall id is in eax.
; A pointer to the parameters in reverse in ebx.
; The return value is in eax.
; Registers are preserved, except for eax.
isr64:
	cli
	push_gpurpose
	
	; Get the syscall id.
	lea accumulator, [accumulator*ptr_sz]
	lea accumulator, [_ZN4obos7process14g_syscallTableE+accumulator]

	push_gpurpose
	call _ZN4obos12EnterSyscallEv
	pop_gpurpose

	pass_par1
	call [accumulator]
	clean_pass_par1

	push_gpurpose
	call _ZN4obos11ExitSyscallEv
	pop_gpurpose

	mov [ret], accumulator

	pop_gpurpose

	mov accumulator, [ret]

	iret