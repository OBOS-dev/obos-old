[BITS 64]

; extern "C" void callDriverIrqHandler(PBYTE newRsp, UINTPTR_T* l4PageMap, void(*irqHandler)());
global callDriverIrqHandler

callDriverIrqHandler:
	push rbp
	mov rbp, rsp
	push r15
	push r14

	mov r14, rsp
	mov rsp, rdi

	mov r15, cr3
	mov cr3, rsi

	call rdx

	mov cr3, r15
	mov rsp, r14

	cli
	pop r14
	pop r15
	leave
	ret