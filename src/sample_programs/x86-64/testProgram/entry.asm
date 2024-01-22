; This file is released to the public domain.
; Example syscall code for oboskrnl on x86-64
; Compile with nasm.

[BITS 64]
[DEFAULT REL]


global _start

global thrStart

section .rodata
path: db "1:/splash.txt",0x00
failureMessage: db "Could not load 1:/splash.txt.",0x10,0x00
failureMessageSz equ $-failureMessage
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
setColourParameters:
.foreground:
dq 0xCCCCCCCC,0
.background:
dq 0,0
.clearConsole:
dq 1,0
consoleOutputParameters:
.str: dq readFileBuf, 0
.sz: dq 0,0
section .text
align 0x1

_start:
    push rbp
    mov rbp, rsp
    sub rsp, 0x10
    ; Stack frame is as follows:
    ; Difference from rbp | Description
    ;                -0x8 | Return address
    ;                 0x0 | Previous rbp
    ;                 0x8 | File handle
    ;                0x10 | File size

    ; Initialize the console.
    mov rax, 25
    xor rdi,rdi
    syscall
    mov rax, 32
    mov rdi, setColourParameters
    syscall
    
    ; Open a file handle to 1:/splash.txt
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
    mov [consoleOutputParameters.sz], rax
    mov rax, 15
    mov rdi, readFileParameters
    syscall
    cmp rax, 0
    jz .failure

    ; Dump the contents of the file.
    mov rax, 26
    mov rdi, consoleOutputParameters
    syscall

    xor rax,rax
    jmp .exit

    ; Set the exit code.
.failure:
    mov qword [consoleOutputParameters.str], failureMessage
    mov qword [consoleOutputParameters.sz], failureMessageSz
    mov rax, 26
    mov rdi, consoleOutputParameters
    syscall
    mov rax, 1
.exit:
    ; Close the file handle.
    push rax
    mov rax, 22
    mov rdi, generalFileQueryParameters
    syscall
    mov rax, 24
    mov rdi, generalFileQueryParameters
    syscall
    pop rax

    ; ExitThread(0);
    push 0
    push rax
    mov rax, 12
    mov rdi, rsp
    syscall
    add rsp, 16

    leave
    ret