; oboskrnl/arch/x86_64/int_handlers.asm

; Copyright (c) 2023 Omar Berrow

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

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
%assign current_isr current_isr + 1
push 0
push %1
jmp isr_common_stub
%endmacro
%macro ISR_ERRCODE 1
global isr%1
isr%1:
%assign current_isr current_isr + 1
push %1
jmp isr_common_stub
%endmacro

%assign current_isr 0

 ISR_NOERRCODE 0
 ISR_NOERRCODE 1
 ISR_NOERRCODE 2
 ISR_NOERRCODE 3
 ISR_NOERRCODE 4
 ISR_NOERRCODE 5
 ISR_NOERRCODE 6
 ISR_NOERRCODE 7
 ISR_ERRCODE   8
 ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31
%rep 224
ISR_NOERRCODE current_isr
%endrep

extern _ZN4obos10g_handlersE

isr_common_stub:
	pushaq

	mov rax, ss
	push rax

	mov rax, [rsp+0x88]
	mov rax, [_ZN4obos10g_handlersE+rax*8]

	test rax, rax
	jz .finished

	mov rdi, rsp
	call rax

.finished:
	
	add rsp, 8

	popaq

	add rsp, 16

	iretq