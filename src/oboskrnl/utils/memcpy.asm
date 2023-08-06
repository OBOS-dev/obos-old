; utils/memcpy.asm
; 
; 2023 Omar Berrow 
;
; This source file (this one specifically, none other, unless otherwise specified by Omar Berrow) is released into the public domain.

[BITS 32]

; void* obos::utils::memcpy(PVOID dest, PCVOID src, SIZE_T size)
[global _ZN4obos5utils6memcpyEPvPKvj]

; void* obos::utils::memzero(PVOID dest, SIZE_T count)
[global _ZN4obos5utils7memzeroEPvj]

; void* obos::utils::dwMemset(DWORD* dest, DWORD val, SIZE_T countDwords);
[global _ZN4obos5utils8dwMemsetEPjjj]

; PVOID obos::utils::memset(PVOID block, UINT32_T ch, SIZE_T size)
[global _ZN4obos5utils6memsetEPvjj]

; SIZE_T obos::utils::strlen(CSTRING str);
[global _ZN4obos5utils6strlenEPKc]

_ZN4obos5utils6memcpyEPvPKvj:
	push ebp
	mov ebp, esp

	mov ecx, [ebp+16]

	test ecx, ecx
	jz .finish

	mov edi, [ebp+8]
	mov esi, [ebp+12]
	
.loop:
	mov al, [esi]
	mov byte [edi], al

	inc esi
	inc edi
	loop .loop

	mov eax, [ebp+8]

.finish:
	leave
	ret
_ZN4obos5utils7memzeroEPvj:
	push ebp
	mov ebp, esp

	mov ecx, [ebp+12]
	
	test ecx, ecx
	jz .finish

	mov edi, [ebp+8]

.loop:
	mov byte [edi], 0

	inc edi
	loop .loop

	mov eax, [ebp+8]

.finish:
	leave
	ret
_ZN4obos5utils8dwMemsetEPjjj:
	push ebp
	mov ebp, esp

	mov ecx, [ebp+16]

	test ecx, ecx
	jz .finish

	mov edi, [ebp+8]

	mov eax, [ebp+12]

.loop:
	mov dword [edi], eax

	add edi, 4
	loop .loop

	mov eax, [ebp+8]

.finish:
	leave
	ret
_ZN4obos5utils6memsetEPvjj:
	push ebp
	mov ebp, esp

	mov ecx, [ebp+16]
	
	test ecx, ecx
	jz .finish

	mov edi, [ebp+8]

	mov dl, [ebp+12]

.loop:
	mov byte [edi], dl

	inc edi
	loop .loop

	mov eax, [ebp+8]

.finish:
	leave
	ret
_ZN4obos5utils6strlenEPKc:
	push ebp
	mov ebp, esp

	mov eax, 0
	mov esi, [ebp+8]

.loop:
	mov dl, byte [esi+eax]

	inc eax

	test dl, dl

	jnz .loop

.finish:
	dec eax

	leave
	ret