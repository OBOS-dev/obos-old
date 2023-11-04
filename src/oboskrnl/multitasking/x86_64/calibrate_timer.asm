; oboskrnl/multitasking/x86_64/calibrate_timer.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 64]

global calibrateTimer
global fpuInit
extern _ZN4obos15g_localAPICAddrE
extern _ZN4obos10g_HPETAddrE
; extern _ZN4obos24RegisterInterruptHandlerEhPFvPNS_15interrupt_frameEE
extern _ZN4obos6thread13configureHPETEm

segment .bss

segment .text

fpuInit:
	fninit
	ret

calibrateTimer:
	push rbp
	mov rbp, rsp
	push rbx
	push r15
	push r14

	mov r15, [_ZN4obos15g_localAPICAddrE]
	mov r14, [_ZN4obos10g_HPETAddrE]

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

	mov dword [r15+0x3E0], 13  ; divisorConfig

	call _ZN4obos6thread13configureHPETEm

	mov dword [r15+0x380], 0xffffffff ; initialCount
	add r14, 0xf0 ; mainCounterValue
.loop:
	mov r11, [r14]
	cmp r11, rax
	jng .loop
	mov r11d, [r15+0x390] ; currentCount
	mov rax, 0xffffffff
	sub rax, r11
	mov dword [r15+0x380], 0 ; initialCount
	
	popfq

.finish:
	pop r14
	pop r15
	pop rbx
	leave
	ret