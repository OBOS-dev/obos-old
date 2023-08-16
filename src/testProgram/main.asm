; testProgram/main.c
;
; Copyright (c) 2023 Omar Berrow

[BITS 32]

segment .data
msg: db 'Hello, world!', 0xD,0xA, 0x0
len: dd 15

segment .text

global _start

_start:
	push 0
	mov ebp, esp

	lea eax, [msg]
	push eax
	mov eax, 0
	lea ebx, [esp]
	int 0x40

	leave
	ret