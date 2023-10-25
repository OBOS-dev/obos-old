; oboskrnl/multitasking/x86_64/taskSwitchImpl.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 64]

segment .bss

blockCallbackStackBottom:
RESB 8192
blockCallbackStack:

segment .text

%macro popaq 0
pop rbp
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rdi
pop rsi
add rsp, 8
pop rbx
pop rdx
pop rcx
pop rax
%endmacro

extern _ZN4obos11SetTSSStackEPv
extern _ZN4obos7SendEOIEv

global _ZN4obos6thread18switchToThreadImplEPNS0_14taskSwitchInfoE
global _ZN4obos6thread25callBlockCallbackOnThreadEPNS0_14taskSwitchInfoEPFbPvS3_ES3_S3_
global idleTask

_ZN4obos6thread18switchToThreadImplEPNS0_14taskSwitchInfoE:
	call _ZN4obos7SendEOIEv
	
	mov rax, [rdi]
	mov cr3, rax
	push rdi
	mov rdi, [rdi+8]
	add rdi, 4096*4
	call _ZN4obos11SetTSSStackEPv
	pop rdi
	add rdi, 16

;	Restore all registers in the interrupt frame
	mov rax, [rdi]
	mov ss, ax

	add rdi, 8

	mov rsp, rdi
	popaq

	add rsp, 16

	iretq
_ZN4obos6thread25callBlockCallbackOnThreadEPNS0_14taskSwitchInfoEPFbPvS3_ES3_S3_:
	push rbp
	mov rbp, rsp
	push r15

	mov rax, cr3
	push rax

	mov r15, rsp
	mov r8, rsi

	mov rsp, blockCallbackStack

	mov rdi, rdx
	mov rsi, rcx
	call r8

	mov rsp, r15
	pop r11
	mov cr3, r11
	
	pop r15
	leave
	ret
idleTask:
	sti
	hlt
	jmp idleTask