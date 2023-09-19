; vfs/x86_64/dInterrupts.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 64]

; extern "C" void callDriverIterateCallback(PBYTE newRsp, 
; UINTPTR_T* l4PageMap,
; void(*iterateCallback)(void(*appendEntry)(CSTRING filename, SIZE_T bufSize)),
; void(*fPar1)(CSTRING filename, SIZE_T bufSize));
global callDriverIterateCallback

callDriverIterateCallback:
	push rbp
	mov rbp, rsp
	push r15
	push r14

	mov r14, rsp
	mov rsp, rdi

	mov r15, cr3
	mov cr3, rsi

	mov rdi, rcx
	call rdx

	mov cr3, r15
	mov rsp, r14

	cli
	pop r14
	pop r15
	leave
	ret