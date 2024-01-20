; oboskrnl/arch/x86_64/int_handlers.asm

; Copyright (c) 2023-2024 Omar Berrow

[BITS 64]

%macro pushaq 0
push rax
; rax has rsp.
mov rax, rsp
add rax, 8
push rcx
push rdx
push rbx
push rax ; Push rsp
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
push qword [rsp+0x88]
push rbp
%endmacro

; Cleans up after pushaq.

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

%macro pushaq_syscall 0
push rax
; rax has rsp.
mov rax, rsp
add rax, 8
push rcx
push rdx
push rbx
push rax ; Push rsp
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
push qword [rsp+0x78]
push rbp
%endmacro

%macro pushaq_syscalli 0
push rax
; rax has rsp.
mov rax, rsp
add rax, 8
push rcx
push rdx
push rbx
push rax ; Push rsp
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
push rcx ; rip
push rbp
%endmacro

%macro popaq_syscall 0
pop rbp
add rsp, 8 ; Skip the pushed rip.
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
add rsp, 8 ; Skip the pushed rsp
pop rbx
add rsp, 8 ; Skip rdx
pop rcx
add rsp, 8 ; Skip rax
%endmacro

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
%assign current_isr current_isr + 1
cld
push 0
push %1
jmp isr_common_stub
%endmacro
%macro ISR_ERRCODE 1
global isr%1
isr%1:
%assign current_isr current_isr + 1
cld
push %1
jmp isr_common_stub
%endmacro

%assign current_isr 0

 ISR_NOERRCODE 0
 ISR_NOERRCODE 1
 ISR_NOERRCODE 2
 ISR_NOERRCODE 3
 ISR_NOERRCODE 4
 ISR_NOERRCODE 5
 ISR_NOERRCODE 6
 ISR_NOERRCODE 7
 ISR_ERRCODE   8
 ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31
%rep 18
ISR_NOERRCODE current_isr
%endrep
%assign current_isr current_isr + 1
%rep 205
ISR_NOERRCODE current_isr
%endrep

extern _ZN4obos10g_handlersE

isr_common_stub:
	pushaq
	mov rbp, rsp

	mov rax, ds
	push rax

;	mov ax, 0x10
;	mov ds, ax
;	mov ss, ax
;	mov es, ax
;	mov fs, ax
;	mov gs, ax

	mov rax, [rsp+0x90]
	cmp rax, 255
	ja .finished
	mov rax, [_ZN4obos10g_handlersE+rax*8]

	swapgs

	test rax, rax
	jz .finished

	mov rdi, rsp
	call rax

.finished:
	
	swapgs

	mov rsp, rbp
	popaq

	add rsp, 16

	iretq

extern _ZN4obos8syscalls14g_syscallTableE
global isr50
; (in) rdi: A pointer to a structure with the parameters.
; (in) rax: The syscall number.
; (out) rax: Lower 64 bits of the return value.
; (out) rdx: Upper 64 bits of the return value.
; Registers are preserved except for rax and rdx.
isr50:
	cld
	pushaq_syscall
	mov rbp, rsp

	mov rax, [rsp+0x80]
	mov rax, [_ZN4obos8syscalls14g_syscallTableE+rax*8]

	swapgs

	mov r15, cr4
	test r15, (1<<21) ; cr4.SMAP[bit 21]
	jz .jump
	stac

.jump:

	test rax,rax
	mov r15, 0xB16B00B1E5DEADBE
	cmovz rax, r15
	mov r15, 0xEF15B00B1E500000
	cmovz rdx, r15
	jz .finish

	; Pass the syscall number into the syscall handler as the first parameter, and the argument pointer in the second.
	mov rsi, rdi
	mov rdi, [rsp+0x80]
	call rax

.finish:

	swapgs
	
	popaq_syscall
	iretq
; (in) rdi: A pointer to a structure with the parameters.
; (in) rax: The syscall number.
; (out) rax: Lower 64 bits of the return value.
; (out) rdx: Upper 64 bits of the return value.
; Registers are preserved except for rax, rdx, rcx, and r9-r11.
section .rodata
panic_format_syscall_kernel: db "Kernel mode thread %d (rip: %p) used syscall!", 0x0A, 0x00
section .text
extern _ZN4obos6logger5panicEPvPKcz
syscall_instruction_handler:
; We need to load a kernel stack.
	swapgs
	; r10 and r9 is the only register we are going to be using before saving all registers.
	mov r10, rsp
	; $RSP = GetCurrentCpuLocal()->currentThread->context.tssStackBottom + 0x4000
	rdgsbase r9
	mov r9, [r9+0x10]
	mov r9, [r9+0x98]
	add r9, 0x4000
	mov rsp, r9

.save:
	pushaq_syscalli
	mov rbp, rsp
	
	; if(GetCurrentCpuLocal()->currentThread->context.ss == 0x10)
	;	obos::logger::panic(nullptr, panic_format_syscall_kernel, GetCurrentCpuLocal()->currentThread->tid, GetCurrentCpuLocal()->currentThread->context.frame.rip);
	; // ...
	mov r15, cr4
	test r15, (1<<21) ; cr4.SMAP[bit 21]
	jz .check_kmode
	stac

.check_kmode:
	rdgsbase r10
	mov r10, [r10+0x10]
	mov r9, [r10+0x100]
	cmp r9, 0x10
	jne .call
	; A kernel-mode thread can't syscall with the "syscall" instruction.
	mov rdi, 0 ; nullptr
	mov rsi, panic_format_syscall_kernel ; The format string.
	mov edx, [r10] ; thr->tid
	; The address of the caller is already in rcx (from the syscall instruction), how nice.
	; mov rcx, rcx
	mov eax, 2 ; Two varargs
	call _ZN4obos6logger5panicEPvPKcz ; Panic!
.call:
	mov rax, [rsp+0x80]
	mov rax, [_ZN4obos8syscalls14g_syscallTableE+rax*8]

	test rax, rax
	mov r15, 0xB16B00B1E5DEADBE
	cmovz rax, r15
	mov r15, 0xEF15B00B1E500000
	cmovz rdx, r15
	jz .finish
	
; Pass the syscall number into the syscall handler as the first parameter, and the argument pointer in the second.
	mov rsi, rdi
	mov rdi, [rsp+0x80]
	call rax

.finish:
	popaq_syscall
	mov rsp, r10

	swapgs
	o64 sysret

%define IA32_EFER  0xC0000080
%define IA32_STAR  0xC0000081
%define IA32_LSTAR 0xC0000082
%define IA32_CSTAR 0xC0000083
%define IA32_FSTAR 0xC0000084
_rdmsr:
	push rbp
	mov rbp, rsp
	
	push rcx
	push rdx
	mov rcx, rdi
	rdmsr
	shl rdx, 32
	or rax, rdx
	pop rdx
	pop rcx
	
	leave
	ret
_wrmsr:
	push rbp
	mov rbp, rsp
	
	push rcx
	push rdx
	push rax
	mov rcx, rdi
	mov rax, rsi
	and eax, 0xffffffff
	mov rdx, rsi
	shr rdx, 32
	wrmsr
	pop rax
	pop rdx
	pop rcx
	
	leave
	ret
global initialize_syscall_instruction
initialize_syscall_instruction:
	push rbp
	mov rbp, rsp
	
	; Set IA32_EFER:SCE = 1
	mov rdi, IA32_EFER
	call _rdmsr
	mov rdi, IA32_EFER
	mov rsi, rax
	or rsi, (1<<0)
	call _wrmsr
	
	; Set the approiate MSRs to the right values to do this when the syscall instruction is invoked:
	; Set CS to 0x08, and SS to 0x10
	; Clear IF,TF,AC, and DF in RFLAGS
	; Jump to syscall_instruction_handler

	mov rdi, IA32_STAR
	mov rsi, 0x1B0000800000000 ; CS: 0x08, SS: 0x10, User CS: 0x1b, User SS: 0x23
	call _wrmsr
	mov rdi, IA32_FSTAR
	mov rsi, 0x40700 ; Clear IF,TF,AC, and DF
	call _wrmsr
	mov rdi, IA32_LSTAR
	mov rsi, syscall_instruction_handler
	call _wrmsr
	
	leave
	ret