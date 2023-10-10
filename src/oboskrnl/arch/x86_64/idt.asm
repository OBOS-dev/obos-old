; oboskrnl/arch/x86_64/idt.asm

; Copyright (c) 2023 Omar Berrow

[BITS 64]

global idtFlush

idtFlush:
	lidt [rdi]
	ret