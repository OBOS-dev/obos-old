; idt.asm
;
; Copyright (c) 2023 Omar Berrow
[BITS 32]

; extern "C" void idtFlush(UINTPTR_T base);
global idtFlush

idtFlush:
	mov eax, [esp+4]
	lidt [eax]
	ret