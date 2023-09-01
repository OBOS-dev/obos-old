[BITS 64]

; segment .data
; instruction_pointer: dq 0
; temp: dq 0

segment .text

global _ZN4obos12multitasking15switchToTaskAsmEv
global _ZN4obos12multitasking20switchToUserModeTaskEv

_ZN4obos12multitasking15switchToTaskAsmEv:
	cli

;	Restore the general purpose registers.

	mov r8, [rax+8]
	mov r9, [rax+16]
	mov r10, [rax+24]
	mov r11, [rax+32]
	mov r12, [rax+40]
	mov r13, [rax+48]
	mov r14, [rax+56]
	mov r15, [rax+64]

	mov rdi, [rax+72]
	mov rsi, [rax+80]
	mov rbp, [rax+88]
	mov rsp, [rax+96]
	mov rbx, [rax+104]
	mov rdx, [rax+112]
	mov rcx, [rax+120]
	; rax is to be the last register to be restored.

	; Store rip at instruction_pointer
	; mov [temp], rax
	; mov rax, [rax+152]
	; mov [instruction_pointer], rax
	; mov rax, [temp]
	
	; Restore rflags
	
	push 0x10
	push qword [rax+96]
	push qword [rax+168]
	push 0x08
	push qword [rax+152]

	mov rax, [rax+128]

	; Using iretq instead of jmp fixes two bugs (probably a lot more too): interrupts being enabled and ruining stuff, the trap bit being set in rflags.
	iretq
_ZN4obos12multitasking20switchToUserModeTaskEv:
	; Unimplemented.
	jmp _ZN4obos12multitasking15switchToTaskAsmEv