[BITS 64]

; segment .data
; instruction_pointer: dq 0
; temp: dq 0

segment .text

global _ZN4obos12multitasking15switchToTaskAsmEv
global _ZN4obos12multitasking20switchToUserModeTaskEv
global callSwitchToTaskImpl
global callBlockCallback
global callExitThreadImpl

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
	
	push 0x23
	push qword [rax+96]
	push qword [rax+168]
	push 0x1B
	push qword [rax+152]

	mov rax, [rax+128]

	iretq
callSwitchToTaskImpl:
	push rbp
	mov rbp, rsp

	mov rsp, rdi
	mov cr3, rsi

	mov rdi, rcx
	call rdx

.failed:
	cli
	hlt
	jmp .failed
callBlockCallback:
	push rbp
	mov rbp, rsp

	mov r12, rsp
	mov r13, cr3

	mov rsp, rdi
	mov cr3, rsi

	mov rdi, rcx
	mov rsi, r8
	call rdx

	mov rsp, r12
	mov cr3, r13

	leave
	ret
callExitThreadImpl:
	push rbp
	mov rbp, rsp

	mov rsp, rdi

	mov rdi, rsi
	call rdx

.failed:
	int 0x30
	jmp .failed