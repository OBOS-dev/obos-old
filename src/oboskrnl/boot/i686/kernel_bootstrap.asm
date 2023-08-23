;   oboskrnl/boot/i686/kernel_boootstrap.asm
;
;   Copyright (c) 2023 Omar Berrow

[BITS 32]

%define FLAGS 7
%define CHECKSUM -(0x1BADB002 + FLAGS)

segment .multiboot.data
align 4
multiboot_header:
    dd 0x1BADB002
    dd FLAGS
    dd CHECKSUM
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
; VGA Linear graphics mode.
    dd 0
    dd 1024
    dd 768
    dd 32
align 4096
boot_page_table1:
    times 4096 db 0
boot_page_directory1:
    times 4096 db 0
align 4
magic_number:
    dd 0xFEFEFEFE

global boot_page_directory1

section .bootstrap_stack nobits
_ZN4obos12stack_bottomE:
    RESB 16384
_ZN4obos9stack_topE:
_ZN4obos15thrstack_bottomE:
    RESB 16384
_ZN4obos12thrstack_topE:

global _ZN4obos9stack_topE
global _ZN4obos12stack_bottomE
global _ZN4obos12thrstack_topE
global _ZN4obos15thrstack_bottomE

extern _ZN4obos5kmainEP14multiboot_infoj
extern _init
extern _fini

segment .multiboot.text
global _start
_start:
    ; Disable interrupts.
    cli

    mov [magic_number], eax

    lea edi, [boot_page_table1+4]
    mov esi, 0x1000
    mov ecx, 1023

.loop:
    mov [edi], esi
    or dword [edi], 3

    add edi, 4
    add esi, 0x1000
    loop .loop

    mov eax, boot_page_table1
    or eax, 3
    mov [boot_page_directory1+0*4], eax
    mov [boot_page_directory1+0x300*4], eax

.enable_paging:
; Load the page directory.
	mov eax, boot_page_directory1
	mov cr3, eax

; Enable paging.
 	mov eax, cr0
 	or eax, 0x80010001
 	mov cr0, eax

    mov eax, [magic_number]

    mov ecx, 0
    mov edi, 0
    mov esi, 0

;   Jump to the bootstrap
    jmp .paging_enabled

segment .text
.paging_enabled:
    mov dword [boot_page_directory1], 0
    ; Setup the stack
    mov esp, _ZN4obos9stack_topE
    xor ebp, ebp

    ; Call the kernel.
    push eax
    push ebx
    call _ZN4obos5kmainEP14multiboot_infoj
    
;   Reset the computer.

_ZN4obos15RestartComputerEv:
[global _ZN4obos15RestartComputerEv]
wait1:
    in   al, 0x64
    test al, 00000010b
    jne  wait1
    
    mov  al, 0xFE
    out  0x64, al

    ; Hold the machine. Shouldn't get hit, but just in case.
    db 0xEB, 0xFE

    ; Shouldn't get hit. Only here for completeness.
    ret