[BITS 64]

global _ZN4obos5utils6strlenEPKc

_ZN4obos5utils6strlenEPKc:
	push rbp
	mov rbp, rsp

	mov rax, 0

.loop:
	mov dl, [rdi+rax]
	inc rax

	test dl,dl
	jnz .loop
.finished:

	dec rax

	leave
	ret