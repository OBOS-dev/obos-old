; arch/x86_64/smp_trampoline.asm
; 
; Copyright (c) 2023-2024 Omar Berrow

[BITS 16]

segment .data

global _strampoline
global _etrampoline
global loadGDT

extern GDT_Ptr

; The start of the trampoline.
_strampoline:
	db 0xEB, 0x38

align 8
gdt:
	dq 0
; 64-bit Code segment.
	dq 0x00209A0000000000
; 64-bit Data segment.
	dq 0x0000920000000000
; 32-bit Code segment.
	dd 0x0000FFFF
	dd 0x00CF9A00
; 32-bit Data segment.
	dd 0x0000FFFF
	dd 0x00CF9200
gdt_end:
gdt_ptr:
%define gdt_ptr_addr 0x30
align 1
	dw gdt_end-gdt-1
	dq 0x8

; The real-mode trampoline.
; We will always be loaded at nullptr, so we can assume some values.
trampoline:

.real_mode:
	cli

; Enter protected-mode
	mov eax, cr0
	or ax, 1 ; Enable protected mode.
	mov cr0, eax
	
	lgdt [gdt_ptr_addr]

	mov sp, 0xFD0
	
	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax

	db 0xE8, 0x00, 0x00 ; call REL16
	pop ax
	add ax, 10
	mov byte [0x01], al

	db 0x9A, 0x00, 0x00, 0x18, 0x00 ; call 0x18:.protected_mode
align 1
.protected_mode:
	[BITS 32]

; Enter 64-bit long mode.

; Enable PAE
	mov eax, cr4
	or eax, (1<<5)
	mov cr4, eax

; Load the page map
	mov eax, [0xff8]
	mov cr3, eax

; Enter long mode.
	mov eax, 0x80000001
	xor ecx,ecx
	cpuid
	xor eax, eax
	mov esi, (1<<11)
	and edx, (1<<20)
	cmovnz eax, esi ; If bit 20 is set
	mov ecx, 0xC0000080
    or eax, (1 << 8) | (1 << 10)
    wrmsr

; Enable paging.
	mov eax, 0x80010001
	mov cr0, eax

	mov esp, 0xFC8
	call .jmp
.jmp:
	pop edi ; The address of .set_cs needs to be in a register that won't be modified by a) cpuid b) the rest of the code.
	add edi, 8
	db 0x6A, 0x08, 0x57, 0xCB ; push 0x08; push edi; retfd

.set_cs:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax

.done:
[BITS 64]
	mov al, [0xFB0]
	test al,al
	jnz .call_procInit

	lgdt [GDT_Ptr]	

; Set "trampoline_has_gdt" to true
	mov qword [0xFB0], 1

	push 0x8
	push rdi
	retfq

.call_procInit:
; Set "trampoline_has_gdt" to false for the next AP.
	mov qword [0xFB0], 0

	mov rdi, [0xFD8]
	mov rsp, [rdi]
	add rsp, 0x2000
	jmp [0xFA8]

.abort:
	cli
	hlt
	jmp .abort

	align 4096
.end:

_etrampoline:
align 1
segment .text
loadGDT:
	lgdt [rdi]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	mov ax, 0x28
	ltr ax

	pop rax
	push 0x8
	push rax
	retfq