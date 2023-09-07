[BITS 32]

; UINT32_T getEflags(void);
global _Z9getEflagsv
global _Z6getEIPv
global getEBP

_Z9getEflagsv:
	push ebp
	mov ebp, esp

	pushfd
	pop eax

	leave
	ret
_Z6getEIPv:
	pop eax
	jmp eax
getEBP:
	mov eax, ebp
	ret