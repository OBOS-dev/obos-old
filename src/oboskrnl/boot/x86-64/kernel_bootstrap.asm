;   oboskrnl/boot/x86-64/kernel_boootstrap.asm
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
boot_page_level3_map:
    times 4096 db 0
boot_page_level4_map:
    times 4096 db 0
align 4
; magic_number:
;     dd 0xFEFEFEFE
; multiboot_struct:
;     dd 0xFEFEFEFE
boot_stack_bottom:
    times 1024 db 0
boot_stack:
IDT:
    .Length dw 0
    .Base dd 0
GDT:
.Null:
    dq 0x0000000000000000
 
.Code:
    dq 0x00209A0000000000
    dq 0x0000920000000000

    dw 0
.Pointer:
    dw $ - GDT - 1
    dd GDT

global boot_page_level4_map

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
_abort_boot:
    in   al, 0x64
    test al, 00000010b
    jne  wait1
    
    mov  al, 0xFE
    out  0x64, al

    db 0xEB, 0xFE

_start:
    ; Disable interrupts.
    cli

    mov esp, boot_stack

    push eax
    push 0
    push ebx
    push 0

    ; Check if we have cpuid.
    pushfd
    pop edx
    and edx, 0x200000
    ; Oh no!
    jz _abort_boot

    ; Check if we're on a x86_64 processor.
    mov eax, 0x80000001
    cpuid
    ; Check bit 29 in edx
    test edx, 0x20000000
    jz _abort_boot

    ; Horray!

    mov dword [boot_page_level4_map], boot_page_level3_map
    or dword [boot_page_level4_map], 3
    mov dword [boot_page_level3_map], boot_page_directory1
    or dword [boot_page_level3_map], 3
    mov dword [boot_page_directory1], boot_page_table1
    or dword [boot_page_directory1], 3

    mov dword [boot_page_level4_map+(511*8)], boot_page_level3_map
    or dword [boot_page_level4_map+(511*8)], 3
    mov dword [boot_page_level3_map+(510*8)], boot_page_directory1
    or dword [boot_page_level3_map+(510*8)], 3

    mov esi, 0x00000003
    mov edi, boot_page_table1
    mov ecx, 512

.loop:
    mov [edi], esi

    add edi, 8
    add esi, 0x1000
    loop .loop

.finish:

    lidt [IDT]

;   Enable PAE.
    mov eax, (1 << 5)
    mov cr4, eax

;   Load the level 4 page map.
    mov eax, boot_page_level4_map
    mov cr3, eax

;   Enter long mode.
    mov ecx, 0xC0000080
    rdmsr
    ; Shouldn't be needed, but just in case
    mov ecx, 0xC0000080
    or eax, (1 << 8) | (1 << 10)
    wrmsr

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    mov ecx, 0xC0000080
    rdmsr

    lgdt [GDT.Pointer]

;   Jump to the bootstrap
    jmp 0x08:.set_cs

.set_cs:
    jmp .paging_enabled

segment .text
[bits 64]
.paging_enabled:

;   One more thing to do...
    mov ax, 0x10
    mov ds, ax
    mov fs, ax
    mov es, ax
    mov ss, ax
    mov gs, ax
; We're done setting up long mode!

;   Restore rax and rbx
    pop rbx
    pop rax

; Setup the stack
    mov rsp, _ZN4obos9stack_topE
    xor rbp, rbp

    ; Call the kernel.
    push rax
    push rbx
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