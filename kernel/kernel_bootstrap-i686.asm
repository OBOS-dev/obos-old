;   kernel_boootstrap-i686.asm
;
;   Copyright (c) 2023 Omar Berrow

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
    dd 1
    dd 80
    dd 25
    dd 0
segment .bss
stack_bottom:
    RESB 16384
stack_top:
stack_bottom_thread:
    RESB 16384
stack_top_thread:
segment .data
parameters:
message: dd 0
sizeof_message: dd 28
exit_code: dd 0

global stack_top
global stack_bottom
global stack_top_thread
global stack_bottom_thread
; global test_program

extern kmain

segment .text
global _start
_start:
    ; Disable interrupts.
    cli

    ; Setup the stack
    mov esp, stack_top
    xor ebp, ebp

    ; Call the kernel.
    push eax
    push ebx
    call kmain

    ; Hold the machine.
.loop: pause
       jmp .loop

    ; Shouldn't get hit. Only here for completeness.
    ret
global idleTask
idleTask:
; Jumps to the same address. (JMP rel8)
    db 0xEB, 0xFE
; test_program:
; ;   Print the message.
;     mov eax, [esp+0x4]
;     mov [message], eax
;     mov eax, 0
;     mov ebx, parameters
;     int 0x40
; ;   Exit the thread.
;     mov eax, 2
;     mov ebx, exit_code
;     int 0x40
; ;   Undefined-Behaviour
;     ret