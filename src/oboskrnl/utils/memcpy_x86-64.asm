; utils/memcpy_86-64.asm
; 
; 2023 Omar Berrow 
;
; This source file (this one specifically, none other, unless otherwise specified by Omar Berrow) is released into the public domain.

[BITS 64]

; void* obos::utils::memcpy(PVOID dest, PCVOID src, SIZE_T size)
[global _ZN4obos5utils6memcpyEPvPKvy]

; void* obos::utils::memzero(PVOID dest, SIZE_T count)
[global _ZN4obos5utils7memzeroEPvy]

; void* obos::utils::dwMemset(DWORD* dest, DWORD val, SIZE_T countDwords);
[global _ZN4obos5utils8dwMemsetEPjjy]

; void* obos::utils::dwMemcpy(DWORD* dest, DWORD* src, SIZE_T countDwords);
[global _ZN4obos5utils8dwMemcpyEPjPKjy]

; PVOID obos::utils::memset(PVOID block, UINT32_T ch, SIZE_T size)
[global _ZN4obos5utils6memsetEPvjj]

; SIZE_T obos::utils::strlen(CSTRING str);
[global _ZN4obos5utils6strlenEPKc]

; To make the compiler happy.
global memset
global memcpy

memcpy:
_ZN4obos5utils6memcpyEPvPKvy:
	push rbp
	mov rbp, rsp
	
	xor rax, rax
	
	mov rcx, rdx

	test rcx, rcx
	jz .finish

	mov r8, rcx
	and r8, 0xFFFFFFFFFFFFFFFB
	jnz .loop
	call _ZN4obos5utils8dwMemcpyEPjPKjy
	jmp .finish

.loop:
	mov al, [rsi]
	mov byte [rdi], al

	inc rsi
	inc rdi
	loop .loop

	mov rax, rdi
	sub rax, rdx

.finish:
	leave
	ret
_ZN4obos5utils7memzeroEPvy:
	push rbp
	mov rbp, rsp

	mov rcx, rsi
	
	test rcx, rcx
	jz .finish

.loop:
	mov byte [rdi], 0

	inc rdi
	loop .loop

	mov rax, rdi
	sub rdi, rdx

.finish:
	leave
	ret
_ZN4obos5utils8dwMemsetEPjjy:
	push rbp
	mov rbp, rsp

	mov rcx, rdx

	test rcx, rcx
	jz .finish

;	mov edi, [ebp+8]

	mov eax, esi

.loop:
	mov dword [rdi], eax

	add rdi, 4
	loop .loop

	mov rax, rdi
	lea r8, [rdx*4]
	sub rax, r8

.finish:
	leave
	ret
_ZN4obos5utils8dwMemcpyEPjPKjy:
	push rbp
	mov rbp, rsp

	mov rcx, rdx

	test rcx, rcx
	jz .finish
.loop:
	mov eax, dword [rsi]
	mov dword [rdi], eax
	
	add rdi, 4
	add rsi, 4
	loop .loop

	mov rax, rdi
	lea r8, [rdx*4]
	sub rax, r8

.finish:
	leave
	ret
memset:
_ZN4obos5utils6memsetEPvjj:
	push rbp
	mov rbp, rsp

	mov rcx, rdx
	
	test rcx, rcx
	jz .finish

.loop:
	mov byte [rdi], sil

	inc edi
	loop .loop

	mov rax, rdi
	sub rdi, rdx

.finish:
	leave
	ret
_ZN4obos5utils6strlenEPKc:
	push rbp
	mov rbp, rsp

	mov rax, 0

.loop:
	mov dl, byte [rdi+rax]

	inc rax

	test dl, dl

	jnz .loop

.finish:
	dec rax

	leave
	ret