; oboskrnl/multitasking/x86_64/taskSwitchImpl.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 64]

global calibrateTimer
extern _ZN4obos15g_localAPICAddrE

getPITCount:
	push rbp
	mov rbp, rsp
	pop rbx

	mov al, 0b1110010
	mov dx, 0x43
	out dx,al

	xor rbx,rbx

	in al, 0x40
	mov bl, al
	in al, 0x40
	mov bh, al
	mov rax, rbx
	
	pop rbx
	leave
	ret

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

	mov dword [r15+0x380], 0xffffffff  ; initialCount

.loop:
	call getPITCount
	test rax,rax
	jnz .loop

	mov edx, dword [r15+0x390] ; currentCount
	mov dword [r15+0x380], 0 ; initialCount
	mov rax, 0xffffffff
	sub rax, rdx
	
	popfq

.finish:
	pop r15
	pop rbx
	leave
	ret