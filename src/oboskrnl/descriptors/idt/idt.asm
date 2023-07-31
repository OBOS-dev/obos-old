; idt.asm
;
; Copyright (c) 2023 Omar Berrow
[BITS 32]

; namespace obos { void idtFlush(UINTPTR_T base); }
global _ZN4obos8idtFlushEj

_ZN4obos8idtFlushEj:
	mov eax, [esp+4]
	lidt [eax]
	ret