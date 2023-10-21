; oboskrnl/multitasking/x86_64/taskSwitchImpl.asm
;
; Copyright (c) 2023 Omar Berrow

[BITS 64]

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

global _ZN4obos6thread18switchToThreadImplEPNS0_14taskSwitchInfoE

_ZN4obos6thread18switchToThreadImplEPNS0_14taskSwitchInfoE:
	mov rax, [rdi]
	mov cr3, rax
	push rdi
	mov rdi, [rdi+8]
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