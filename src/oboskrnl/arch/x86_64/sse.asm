; arch/x86_64/sse.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 64]

global _ZN4obos7initSSEEv

_ZN4obos7initSSEEv:
	mov rax, cr4
	or rax, (1<<9)|(1<<10)
	mov cr4, rax
	mov rax, cr0
	and eax, ~(1<<2)
	or eax, (1<<1)
	mov cr0, rax
	ret