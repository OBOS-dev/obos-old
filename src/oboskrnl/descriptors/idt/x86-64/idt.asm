; idt.asm
;
; Copyright (c) 2023 Omar Berrow
[BITS 64]

; namespace obos { void idtFlush(UINTPTR_T base); }
global _ZN4obos8idtFlushEy

_ZN4obos8idtFlushEy:
	lidt [rdi]
	ret