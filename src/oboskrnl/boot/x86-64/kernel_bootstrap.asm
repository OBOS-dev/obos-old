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
boot_page_table2:
    times 4096 db 0
boot_page_table3:
    times 4096 db 0
boot_page_directory1:
    times 4096 db 0
boot_page_level3_map:
    times 4096 db 0
boot_page_level3_map1:
    times 4096 db 0
boot_page_level4_map:
    times 4096 db 0
align 4
magic_number:
    dd 0xFEFEFEFE
multiboot_struct:
    dd 0xFEFEFEFE
boot_stack_bottom:
    times 1024 db 0
boot_stack:
IDTPtr:
    .Length: dw 0
    .Base: dd 0
GDT:
.Null:
    dq 0x0000000000000000
 
.Code:
    dq 0x00209A0000000000
    dq 0x0000920000000000

    dw 0
GDTPointer:
    dw $ - GDT - 1
    dq GDT

global boot_page_level4_map
global boot_page_level3_map
global boot_page_table1
global idleTask

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
extern __stack_chk_guard

segment .multiboot.text
global _start
_abort_boot:
    in   al, 0x64
    test al, 00000010b
    jne  wait1
    
    mov  al, 0xFE
    out  0x64, al

    db 0xEB, 0xFE
.loop:
    hlt
    jmp .loop
_start:
    ; Disable interrupts.
    cli
    
    mov esp, boot_stack

    lidt [IDTPtr]

    mov [magic_number], eax
    mov [multiboot_struct], ebx
    
    ; Check if we have cpuid.
    ; pushfd
    ; pop edx
    ; test edx, 0x200000
    ; ; Oh no!
    ; mov eax, 0x80808080
    ; jz _abort_boot

    ; Don't check if we have cpuid because ain't no one is going to run this kernel on an i386.
    ; and if we don't check it will have the same effect, rebooting.

    ; In the x86 architecture, the CPUID instruction (identified by a CPUID opcode) 
    ; is a processor supplementary instruction (its name derived from CPU Identification) allowing software to discover details of the processor. 
    ; It was introduced by Intel in 1993 with the launch of the Pentium and SL-enhanced 486 processors.

    ; Check if we're on a x86_64 processor.
    mov eax, 0x80000001
    cpuid
    ; Check bit 29 in edx
    test edx, 0x20000000
    mov eax, 0xFFFFFFFF
    jz _abort_boot

    ; Horray!

    mov dword [boot_page_level4_map], boot_page_level3_map
    or dword [boot_page_level4_map], 3
    
    mov dword [boot_page_level3_map], boot_page_directory1
    or dword [boot_page_level3_map], 3
    
    mov dword [boot_page_directory1], boot_page_table1
    or dword [boot_page_directory1], 3
    
    mov dword [boot_page_directory1+8], boot_page_table2
    or dword [boot_page_directory1], 3
    
    mov dword [boot_page_directory1+16], boot_page_table3
    or dword [boot_page_directory1], 3

    mov dword [boot_page_level4_map+(511*8)], boot_page_level3_map1
    or dword [boot_page_level4_map+(511*8)], 3

    mov dword [boot_page_level3_map1+(510*8)], boot_page_directory1
    or dword [boot_page_level3_map1+(510*8)], 3

;   Fill in 3 page tables (6 MiB).

    mov esi, 0x00000003
    mov edi, boot_page_table1
    mov ecx, (512*3)

.loop:
    mov [edi], esi

    add edi, 8
    add esi, 0x1000
    loop .loop

.finish:

;   Enable PAE.
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

;   Load the level 4 page map.
    mov eax, boot_page_level4_map
    mov cr3, eax

;   Enter long mode.
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8) | (1 << 10)
    wrmsr

    mov eax, cr0
    or eax, 0x80010001
    mov cr0, eax

    lgdt [GDTPointer]

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
    mov rax, 0
    mov rbx, 0
    mov eax, [magic_number]
    mov ebx, [multiboot_struct]

; Setup the stack
    mov rsp, _ZN4obos9stack_topE
    xor rbp, rbp

    ; Call the kernel.
    mov rsi, rax
    mov rdi, rbx
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
idleTask:
    mov al, 0x20
    out 0x20, al
    out 0xA0, al
.loop:
    sti
    hlt 
    jmp .loop