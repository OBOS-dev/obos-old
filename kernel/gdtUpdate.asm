;   gdtUpdate.asm
;   
;   Copyright (c) 2023 Omar Berrow

[BITS 32]

segment .text

global gdtUpdate
global tssUpdate
global idtUpdate

extern defaultInterruptHandler
extern syscall_handler
extern driver_syscall_handler

gdtr DW 0
     DD 0
 
gdtUpdate:
    mov eax, [esp+4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret
tssUpdate:
    mov ax, 0x2B
    ltr ax
    ret
isr_common_stub:
   pusha                    ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

   mov ax, ds               ; Lower 16-bits of eax = ds.
   push eax                 ; save the data segment descriptor

   mov ax, 0x10  ; load the kernel data segment descriptor
   mov ds, ax
   mov es, ax
   mov fs, ax
   mov gs, ax

   call defaultInterruptHandler

   pop eax        ; reload the original data segment descriptor
   mov ds, ax
   mov es, ax
   mov fs, ax
   mov gs, ax

   popa                     ; Pops edi,esi,ebp...
   add esp, 8     ; Cleans up the pushed error code and pushed ISR number
   sti
   iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

%macro ISR_NOERRCODE 1
  [GLOBAL isr%1]
  isr%1:
    cli
    push byte 0
    push %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
  [GLOBAL isr%1]
  isr%1:
    cli
    push %1
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
ISR_ERRCODE   17
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
ISR_NOERRCODE 29
ISR_NOERRCODE 30
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
; Used to call the scheduler.
ISR_NOERRCODE 48
segment .data
    temp: dd 0
segment .text
extern EnterKernelSection
extern LeaveKernelSection
; Syscalls
; The syscall number is in eax, and a pointer to a continuous block of memory with the arguments is in ebx, and the return value is stored in eax.
global isr64
isr64:
    mov [temp], eax

;   Enter a kernel section so the syscall doesn't get interrupted.
    call EnterKernelSection
    
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov eax, [temp]

    push ebx
    push eax
    
    call syscall_handler
    
    pop eax
    pop ebx
    
    mov [temp], eax

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call LeaveKernelSection

    mov eax, [temp]
    iret
; The syscall number is in eax, a pointer to a continuous block of memory with the arguments is in ebx, ecx having the driver id, and the return value is stored in eax.
global isr80
isr80:
    mov [temp], eax

    call EnterKernelSection
    
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov eax, [temp]

    push ebx
    push ecx
    push eax
    
    call driver_syscall_handler
    
    mov [temp], eax
    ;  Use lea instead of add, as it's a bit faster.
    lea esp, [esp+12]

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call LeaveKernelSection

    mov eax, [temp]
    iret
; TODO: Make a linux syscall layer so we can support linux natively.
; ISR_NOERRCODE 128

idtUpdate:
   mov eax, [esp+4]
   lidt [eax]
   ret