[BITS 32]

; UINT32_T getEflags(void);
global _Z9getEflagsv

_Z9getEflagsv:
	push ebp
	mov ebp, esp

	pushfd
	pop eax

	leave
	ret