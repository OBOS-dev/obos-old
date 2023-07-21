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
global switchTaskToAsmUserMode
global OBOS_ThreadEntryPoint
extern ExitThread

switchTaskToAsm:
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
    ; Restore the pointer in eax before.
    mov eax, [tmp]

    ; Set eax to its value.
    mov eax, [tmp]
    mov eax, [eax+48]
    jmp dword [instruction_pointer]
switchTaskToAsmUserMode:
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

    mov ax, (4 * 8) | 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax

    ; Restore the pointer in eax before.
    mov eax, [tmp]

;   Save the stack pointer.
    mov eax, esp

    push (4 * 8) | 3
    push eax
    sti
    pushfd
    push (3 * 8) | 3
    push dword [instruction_pointer]

    ; Set eax to its value.
    mov eax, [tmp]
    mov eax, [eax+48]

;   Switch to user mode (dun dun dun!)
    iret
; eax has the thread entry point, and edi has the user data. This function does not return.
OBOS_ThreadEntryPoint:
    push edi
    call eax
    pop edi
    push eax
    ; Exit the thread.
    mov eax, 2
    mov ebx, esp
    int 0x40
    ret