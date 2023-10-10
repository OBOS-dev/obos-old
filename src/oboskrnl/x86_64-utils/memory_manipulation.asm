[BITS 64]

global _ZN4obos5utils6strlenEPKc
global _ZN4obos5utils7memzeroEPvm

_ZN4obos5utils7memzeroEPvm:
	push rbp
	mov rbp, rsp

	mov rcx, rsi

	test rcx,rcx
	jz .finish

	mov rax, rdi

.loop:
	mov byte [rdi], 0

	inc rdi
	loop .loop

.finish:

	leave
	ret

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