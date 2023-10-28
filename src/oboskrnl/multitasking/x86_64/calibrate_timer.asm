; oboskrnl/multitasking/x86_64/taskSwitchImpl.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 64]

global calibrateTimer
extern _ZN4obos15g_localAPICAddrE
extern _ZN4obos24RegisterInterruptHandlerEhPFvPNS_15interrupt_frameEE

segment .bss

currentCount: 
	resd 1

segment .text

pitInterrupt:
	mov r15, [_ZN4obos15g_localAPICAddrE]
	mov edx, dword [r15+0x390] ; currentCount
	mov [currentCount], edx
	mov dword [r15+0x380], 0 ; initialCount
	mov dword [r15+0xb0], 0 ; EOI
	mov al, 0x20
	out 0x20, al
	mov al, 0xff
	out 0x21, al
	out 0xA1, al
	ret

; getPITCount:
; 	push rbp
; 	mov rbp, rsp
; 	pop rbx
; 
; 	mov al, 0b1110010
; 	mov dx, 0x43
; 	out dx,al
; 
; 	xor rbx,rbx
; 
; 	in al, 0x40
; 	mov bl, al
; 	in al, 0x40
; 	mov bh, al
; 	mov rax, rbx
; 	
; 	pop rbx
; 	leave
; 	ret

calibrateTimer:
	push rbp
	mov rbp, rsp
	push rbx
	push r15

	mov r15, [_ZN4obos15g_localAPICAddrE]

	xor rax,rax
	xor rcx,rcx
	cpuid
	test rax, 0x15
	jnz .noCpuid21

.hasCpuid21:
	xor rcx, rcx
	cpuid
	mov rax, rcx
	xor rdx,rdx
	div rdi
	jmp .finish

.noCpuid21:
	pushfq
	cli

	mov dword [r15+0xb0], 0 ; EOI
	mov dword [r15+0x3E0], 13  ; divisorConfig

	mov al, 0x30
	out 0x43, al
	mov rax, 1193182
	xor rdx,rdx
	div rdi
	push rdi
	mov rdi, rax
	mov al, dl
	out 0x40, al
	mov al, dh
	out 0x40, al
	pop rdi

	push rdi
	mov rdi, 0x20
	mov rsi, pitInterrupt
	call _ZN4obos24RegisterInterruptHandlerEhPFvPNS_15interrupt_frameEE
	pop rdi

.retry:

	mov al, 0xfe
	out 0x21, al
	mov al, 0xff
	out 0xA1, al

	mov dword [r15+0x380], 0xffffffff ; initialCount
	
	; It's pretty hard to wait for the interrupt when interrupts are disabled...
	sti
	; Wait for the interrupt.
	hlt
	cli

	; pitInterrupt disables the pic
	; mov al, 0xff
	; out 0x21, al
	; out 0xA1, al

	mov edx, dword [currentCount] ; currentCount
	test edx, 0xffffffff
	je .retry
	mov rax, 0xffffffff
	sub rax, rdx
	
	popfq

.finish:
	pop r15
	pop rbx
	leave
	ret