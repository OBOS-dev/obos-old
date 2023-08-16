[BITS 32]

segment .data
instruction_pointer: dd 0
temp: dd 0

segment .text

[global _ZN4obos12multitasking15switchToTaskAsmEv]
[global _ZN4obos12multitasking20switchToUserModeTaskEv]

_ZN4obos12multitasking15switchToTaskAsmEv:
    mov edi, [eax+4]
    mov esi, [eax+8]
    mov ebp, [eax+12]
    mov ebx, [eax+20]
    mov edx, [eax+24]
    mov ecx, [eax+28]

    mov [temp], eax
    mov eax, [eax+44]
    mov [instruction_pointer], eax
    mov eax, [temp]
    
    mov esp, [eax+16]

    push dword [eax+52]
    popfd
    
    mov eax, [eax+32]

    jmp [instruction_pointer]
_ZN4obos12multitasking20switchToUserModeTaskEv:
    mov edi, [eax+4]
    mov esi, [eax+8]
    mov ebp, [eax+12]
    mov ebx, [eax+20]
    mov edx, [eax+24]
    mov ecx, [eax+28]

    mov [temp], eax
    mov eax, [eax+44]
    mov [instruction_pointer], eax
;   mov eax, [temp]
    
    mov ax, 35
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov eax, [temp]
    ; Move saved esp to eax.
    mov eax, [eax+16]

    push 35
    push eax
    mov eax, [temp]
    push dword [eax+52]
    push 27
    push dword [instruction_pointer]
    ; Move the saved eax to eax.
    mov eax, [eax+32]
    iret