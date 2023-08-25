[BITS 64]

; UINT32_T getEflags(void);
global _Z9getEflagsv
global _Z6getEIPv
global getEBP
global _ZN4obos6memory19GetPageFaultAddressEv
global _ZN4obos6memory8tlbFlushEy

_Z9getEflagsv:
	push rbp
	mov rbp, rsp

	pushfq
	pop rax

	leave
	ret
_Z6getEIPv:
	pop rax
	jmp rax
getEBP:
	mov rax, rbp
	ret
_ZN4obos6memory19GetPageFaultAddressEv:
	mov rax, cr2
	ret
_ZN4obos6memory8tlbFlushEy:
	invlpg [rdi]
	ret