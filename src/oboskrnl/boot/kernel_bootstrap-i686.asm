;   kernel_boootstrap-i686.asm
;
;   Copyright (c) 2023 Omar Berrow
[BITS 32]

%define FLAGS 7
%define CHECKSUM -(0x1BADB002 + FLAGS)

segment .multiboot
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
    dd 600
    dd 300
    dd 0
segment .bss
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
; global test_program

extern _ZN4obos5kmainEP14multiboot_infoj
extern _init
extern _fini

segment .text
global _start
global nullptrStub
_start:
    ; Disable interrupts.
    cli

    ; Setup the stack
    mov esp, _ZN4obos9stack_topE
    xor ebp, ebp

    ; Call the kernel.
    push eax
    push ebx
    call _ZN4obos5kmainEP14multiboot_infoj
    
    ; Hold the machine.
    db 0xEB, 0xFE

    ; Shouldn't get hit. Only here for completeness.
    ret