; This file is released to the public domain.
; Example syscall code for oboskrnl on x86-64
; Compile with nasm.

[BITS 64]
[DEFAULT REL]


global _start

global thrStart

section .rodata
path: db "1:/test.txt",0x00
section .bss
readFileBuf:
resb 4096
section .data
align 16
openFileParameters:
.handle:
dq 0,0
.path:
dq path,0
.options:
dq 0b1,0
readFileParameters:
.handle:
dq 0,0
.data:
dq readFileBuf,0
.nToRead:
dq 0,0
.peek:
dq 0,0
generalFileQueryParameters:
.handle:
dq 0,0
section .text
align 0x1

_start:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    ; Stack frame is as follows:
    ; Difference from rbp | Description
    ;                -0x8 | Return address
    ;                 0x0 | Previous rbp
    ;                 0x8 | File handle
    ;                0x10 | File size

    ; Open a file handle to 1:/test.txt
    mov rax, 13
    xor rdi,rdi
    syscall
    mov [rbp-0x8], rax
    mov [openFileParameters.handle], rax
    mov [readFileParameters.handle], rax
    mov rax, 14
    mov rdi, openFileParameters
    syscall
    cmp rax, 0
    jz .failure

    ; Read the file.
    mov rax, 19
    mov rdi, generalFileQueryParameters
    syscall
    cmp rax, 0xffffffffffffffff
    jz .failure
    mov [rbp-0x10], rax
    and rax, 0xfff ; Ensure the count to read is less than 0x1000
    mov [readFileParameters.nToRead], rax
    mov rax, 15
    mov rdi, readFileParameters
    syscall
    cmp rax, 0
    jz .failure

    xor rax,rax
    jmp .exit

.failure:
    ; Close the handle.
    mov rax, 22
    mov rdi, generalFileQueryParameters
    syscall
    mov rax, 24
    mov rdi, generalFileQueryParameters
    syscall
    ; Set the exit code.
    mov rax, 1
.exit:
    ; ExitThread(0);
    push 0
    push rax
    mov rax, 12
    mov rdi, rsp
    syscall
    add rsp, 16

    leave
    ret