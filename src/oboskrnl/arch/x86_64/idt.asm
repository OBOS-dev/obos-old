; oboskrnl/arch/x86_64/idt.asm

; Copyright (c) 2023-2024 Omar Berrow

[BITS 64]

section .bss
align 1
bad_idt:
	dq 0
	dw 0
section .text

global idtFlush
global reset_idt

idtFlush:
	lidt [rdi]
	ret
reset_idt:
	lidt [bad_idt]
	ret