[BITS 64]

global _ZN4obos5utils6strlenEPKc
global _ZN4obos5utils14strCountToCharEPKccb
global _ZN4obos5utils7memzeroEPvm
global _ZN4obos5utils8dwMemcpyEPjPKjm
global _ZN4obos5utils8dwMemsetEPjjm
global _ZN4obos5utils6memcmpEPKvS2_m
global _ZN4obos5utils6memcmpEPKvjm
global _ZN4obos5utils6memcpyEPvPKvm
global _ZN4obos3vfs20_VectorizedMemcpy64BEPvPKvm

_ZN4obos5utils8dwMemcpyEPjPKjm:
	push rbp
	mov rbp, rsp

	xor rax, rax

	mov rax, rdi

	mov rcx, rdx
	rep movsd
	
	leave
	ret
_ZN4obos5utils8dwMemsetEPjjm:
	push rdi
	mov rcx, rdx
	mov eax, esi
	rep stosd
	pop rax
	ret

_ZN4obos5utils6memcpyEPvPKvm:
	push rbp
	mov rbp, rsp

	xor rax, rax

	mov rax, rdi

	mov rcx, rdx
	rep movsb
	
	leave
	ret

_ZN4obos5utils7memzeroEPvm:
	push rbp
	mov rbp, rsp

	mov rcx, rsi

	push rdi

	xor rax,rax
	rep stosb

.finish:

	pop rax

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

_ZN4obos5utils6memcmpEPKvjm:
	push rbp
	mov rbp, rsp

	mov rcx, rdx
	test rcx, rcx
	jz .finish

.loop:
	mov r8b, byte [rdi]
	sub r8b, sil
	jne .finish

	inc rdi
	loop .loop

.finish:

	test rcx,rcx
	setz al

	leave
	ret

_ZN4obos5utils14strCountToCharEPKccb:
	push rbp
	mov rbp, rsp

	xor rax,rax
	
	not dl
	and dl, 0x1

.loop:
	mov cl, [rdi+rax]
	inc rax

	cmp cl, dl
	jz .finished
	cmp cl,sil
	jne .loop
.finished:

	dec rax

	leave
	ret

_ZN4obos5utils6strlenEPKc:
	xor sil, sil
	xor dl, dl
	call _ZN4obos5utils14strCountToCharEPKccb
	ret
_ZN4obos3vfs20_VectorizedMemcpy64BEPvPKvm:
	push rbp
	mov rbp, rsp

	push rdi

	mov rax, rdx
	xor rdx,rdx
	mov rcx, 64
	mul rcx
	mov rcx, rax
	rep movsb

	pop rdi

	mov rsp, rbp
	pop rbp
	ret