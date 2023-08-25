[BITS 64]

; Like the pushad instruction, but for 64-bit long mode, and includes r8-r15.

%macro pushaq 0
push rax
; rax has rsp.
mov rax, rsp
add rax, 8
push rcx
push rdx
push rbx
push rax ; Push rsp
push rbp
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
%endmacro

; Cleans up after pushaq.

%macro popaq 0
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
pop rbp
add esp, 8
pop rbx
pop rdx
pop rcx
pop rax
%endmacro

%macro ISR_NOERRCODE 1  ; define a macro, taking one parameter
  [GLOBAL isr%1]        ; %1 accesses the first parameter.
  [BITS 64]
  isr%1:
    cli
    push byte 0
    push byte %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
  [GLOBAL isr%1]
  isr%1:
    cli
    push byte %1
    jmp isr_common_stub
%endmacro

segment .text

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
ISR_NOERRCODE 17
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
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47
ISR_NOERRCODE 48

segment .data
extern _ZN4obos19g_interruptHandlersE

segment .text

extern defaultInterruptHandler

global isr_common_stub

isr_common_stub:
    pushfq

    mov rax, 0
    mov ax, ds
    push rax

;   Push a pointer to the interrupt frame.
    mov rax, rsp
    add rax, 56
    mov rax, [rax]
    lea rax, [rax*8]

    test rax, rax
    jz .finished

    call [rax]

.finished:

    ; Pop the pointer.
    lea rsp, [esp+8]
    
;   Restore the pushed registers.

    pop rax
    mov ds, ax

    popaq

;   Pop the interrupt number and error code.
    lea rsp, [esp+16]

;   Finally return from the interrupt.
    iretq