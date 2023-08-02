[BITS 32]

segment .data
instruction_pointer: dd 0
temp: dd 0

segment .text

[global _ZN4obos12multitasking15switchToTaskAsmEv]

_ZN4obos12multitasking15switchToTaskAsmEv:
    push dword [eax+52]
    popfd

    mov edi, [eax+4]
    mov esi, [eax+8]
    mov ebp, [eax+12]
    mov ebx, [eax+20]
    mov edx, [eax+24]
    mov ecx, [eax+28]

    ; Load frame->eip into instruction_pointer.
    mov [temp], eax
    mov eax, [eax+44]
    mov [instruction_pointer], eax
    mov eax, [temp]

    mov esp, [eax+56]
    mov eax, [eax+32]

    jmp [instruction_pointer]