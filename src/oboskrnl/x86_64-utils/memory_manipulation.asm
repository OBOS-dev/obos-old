[BITS 64]

global _ZN4obos5utils6strlenEPKc
global _ZN4obos5utils7memzeroEPvm
global _ZN4obos5utils8dwMemcpyEPjPKjm
global _ZN4obos5utils8dwMemsetEPjjm
global _ZN4obos5utils6memcmpEPKvS2_m

_ZN4obos5utils8dwMemcpyEPjPKjm:
	push rbp
	mov rbp, rsp

	xor rax, rax

	mov rcx, rdx
	test rcx, rcx
	jz .finish
	
	test rdi, rsi
	jz .finish

	mov rax, rdi
.loop:
	mov r8d, [rsi]
	mov [rdi], r8d

	add rdi, 4
	add rsi, 4
	loop .loop

.finish:
	
	leave
	ret
_ZN4obos5utils8dwMemsetEPjjm:
	push rbp
	mov rbp, rsp

	xor rax, rax

	mov rcx, rdx
	test rcx, rcx
	jz .finish
	
	mov rax, rdi
.loop:
	mov [rdi], rsi

	add rdi, 4
	loop .loop

.finish:
	
	leave
	ret

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

_ZN4obos5utils6memcmpEPKvS2_m:
	push rbp
	mov rbp, rsp

	mov rcx, rdx
	test rcx,rcx
	jz .finish

.loop:
	mov r8b, [rdi]
	mov r9b, [rsi]
	sub r8b, r9b
	jne .finish

	inc rdi
	inc rsi
	loop .loop

.finish:
	
	test rcx, rcx
	setz al

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