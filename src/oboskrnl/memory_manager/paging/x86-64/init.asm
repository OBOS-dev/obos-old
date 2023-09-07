;   oboskrnl/memory_manager/x86-64/init.asm
;
;	Copyright (c) 2023 Omar Berrow

[BITS 64]

global _ZN4obos6memory25CPUSupportsExecuteDisableEv

_ZN4obos6memory25CPUSupportsExecuteDisableEv:
	push rbp
    mov rbp, rsp
    
    mov rax, 0
    mov rcx, 0

    mov ecx, 0xC0000080
    rdmsr
    shr eax, 11
    and eax, 1
    
    leave
    ret