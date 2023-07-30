[BITS 32]

%macro ISR_NOERRCODE 1  ; define a macro, taking one parameter
  [GLOBAL isr%1]        ; %1 accesses the first parameter.
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

segment data
global g_interruptHandlers
g_interruptHandlers:
    times 256-($-$$) dd 0
segment text

extern defaultInterruptHandler

isr_common_stub:
    pushad
    mov ax, ds
    push eax

;   Push a pointer to the interrupt frame.
    mov eax, esp
    mov ebx, [esp+36]
    push eax
    lea ebx, [ebx*4]
    lea eax, [g_interruptHandlers+ebx]
    call [eax]
    
    lea esp, [esp+4]
    pop eax
    mov ds, ax

;   Restore the pushed registers.
    popad
;   Pop the interrupt number and error code.
    lea esp, [esp+8]
;   Finally return from the interrupt.
    iret