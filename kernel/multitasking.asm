; multitasking.asm
; 
; Copyright (c) 2023 Omar Berroww

; // Warning: Function doesn't return.
; void switchTaskToAsm(struct thr_registers* registers);
[BITS 32]

segment .data
    instruction_pointer: dd 0
    tmp: dd 0
segment .text
global switchTaskToAsm
global OBOS_ThreadEntryPoint
extern ExitThread

switchTaskToAsm:
    ; mov ds, [ebp+4]
    ; mov es, [ebp+4]
    ; mov fs, [ebp+4]
    ; mov gs, [ebp+4]
    ; mov ss, [ebp+4]
    ; mov cs, [ebp+12]
    push dword [eax+16]
    popfd
    mov edi, [eax+24]
    mov esi, [eax+28]
    mov esp, [eax+32]
    mov ebx, [eax+36]
    mov edx, [eax+40]
    mov ecx, [eax+44]
    ; Set eax to 'eip.'
    mov [tmp], eax
    mov ebp, [eax]
    mov eax, [eax+8]
    mov [instruction_pointer], eax
    ; Set eax to its value.
    mov eax, [tmp]
    mov eax, [eax+48]
    sti
    jmp dword [instruction_pointer]
    ret
; eax has the thread entry point, and edi has the user data. This function does not return.
OBOS_ThreadEntryPoint:
    push edi
    call eax
    pop edi
    push eax
    call ExitThread
    ret