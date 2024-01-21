; oboskrnl/multitasking/x86_64/taskSwitchImpl.asm
;
; Copyright (c) 2023-2024 Omar Berrow

[BITS 64]

segment .bss

segment .text

%macro popaq 0
pop rbp
add rsp, 8
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
extern _ZN4obos6SetISTEPv
extern _ZN4obos7SendEOIEv
extern _ZN4obos5rdmsrEj

global _ZN4obos6thread18switchToThreadImplEPNS0_14taskSwitchInfoEPNS0_6ThreadE
global _ZN4obos6thread25callBlockCallbackOnThreadEPNS0_14taskSwitchInfoEPFbPvS3_ES3_S3_
global idleTask
global _callScheduler

segment .sched_text
_ZN4obos6thread18switchToThreadImplEPNS0_14taskSwitchInfoEPNS0_6ThreadE:
; rsp = GetCurrentCpuLocalPtr()->temp_stack.addr + GetCurrentCpuLocalPtr()->temp_stack.size
; This code is commented because _callScheduler switches to the cpu temporary, so another switch would be unneccessary.
	; push rdi
	; mov rdi, 0xC0000101
	; call _ZN4obos5rdmsrEj
	; pop rdi
	; add rax, 0x28
	; mov rsp, [rax]
	; add rsp, qword [rax+8]

	call _ZN4obos7SendEOIEv
	
	mov rax, [rdi]
	mov cr3, rax
	push rdi
	mov rdi, [rdi+8]
	add rdi, 4096*4
	mov rbx, rdi
	call _ZN4obos11SetTSSStackEPv
	mov rdi, rbx
	call _ZN4obos6SetISTEPv
	pop rdi
	add rdi, 16

;	Restore all registers in the interrupt frame
	; mov rax, [rdi]
	; mov ss, ax

	add rdi, 8

	mov rsp, rdi
	popaq

	add rsp, 16

	fxrstor [rsp+0x30]

	iretq
_ZN4obos6thread25callBlockCallbackOnThreadEPNS0_14taskSwitchInfoEPFbPvS3_ES3_S3_:
	push rbp
	mov rbp, rsp
	
; rsp = context->tssStackBottom + 0x4000
	mov rsp, [rdi+8]
	add rsp, 0x4000
	
	mov r10, cr3
	
	mov rax, [rdi]
	mov cr3, rax
	
	push r10

	mov r8, rsi

	mov rdi, rdx
	mov rsi, rcx
	call r8

	pop r11
	mov cr3, r11

	mov rsp, rbp
	pop rbp
	ret
extern _ZN4obos6thread8scheduleEv
_callScheduler:
	push rbp
	mov rbp, rsp

	; Load the temporary stack.
	mov rdi, 0xC0000101
	call _ZN4obos5rdmsrEj
	add rax, 0x28
	mov rsp, [rax]
	add rsp, qword [rax+8]
	call _ZN4obos6thread8scheduleEv

	mov rsp, rbp
	pop rbp
	ret
segment .text
global _ZN4obos6thread21getCurrentCpuLocalPtrEv
_ZN4obos6thread21getCurrentCpuLocalPtrEv:
; Note even if we check if rdgsbase is enabled, it is still a system requirement.
; See int_handlers.asm
	mov rax, cr4
	test rax, (1<<16)
	jz .rdmsr
	rdgsbase rax
	jmp .done
.rdmsr:
	mov rcx, 0xC0000101
	rdmsr
	shl rdx, 32
	or rax, rdx
.done:
	ret
idleTask:
	sti
	hlt
 	jmp idleTask