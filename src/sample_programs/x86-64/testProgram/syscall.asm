; This file is released to the public domain.
; Example syscall code for oboskrnl on x86-64
; Compile with nasm.

[BITS 64]

; uintptr_t syscall(uint64_t syscall, void* args);
global syscall

extern main

syscall:
	push rbp
	mov rbp, rsp
	push rbx
	push r12 
	push r13 
	push r14
	push r15

	mov rax, rdi
	mov rdi, rsi
	syscall

	pop r15
	pop r14
	pop r13 
	pop r12 
	pop rbx
	leave
	ret