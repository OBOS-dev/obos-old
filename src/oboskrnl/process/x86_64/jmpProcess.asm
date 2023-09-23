[BITS 64]

; extern "C" void jmpProcess(PVOID par1, VOID(*entry)(PVOID entry))
[global jmpProcess]

jmpProcess:
	xor rbp, rbp

	mov rax, rsp
	push 0x23 ; User mode ds
	push rax ; rsp
	pushfq
	pop rax
	or rax, (1 << 9) ; Set bit 9 (IF).
	push rax ; RFLAGS with interrupts enabled
	push 0x1B ; User mode cs
	push rsi ; IP
	iretq