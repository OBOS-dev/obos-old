; oboskrnl/syscalls/interrupt.asm
;
; Copyright (c) 2023 Omar Berrow
[BITS 32]


; extern "C" void isr64();
[global isr64]

segment .data
__esp: dd 0

segment .text
extern _ZN4obos7process14g_syscallTableE


; Dispatch program syscalls.
; The syscall id is in eax.
; A pointer to the arguments in reverse order in ebx.
; The return value is in eax.
; Registers are NOT preserved.
isr64:
	cli

	mov [__esp], esp

	; Get the syscall id.
	lea eax, [eax*4]
	lea eax, [_ZN4obos7process14g_syscallTableE+eax]

	mov esp, ebx
	call [eax]

	mov esp, [__esp]

	iret