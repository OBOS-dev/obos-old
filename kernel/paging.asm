[BITS 32]

; void loadPageDirectory(UINT32_T* address);
; // A page directory MUST be loaded before calling, otherwise, bad stuff could happen.
; void enablePaging();
global loadPageDirectory
global enablePaging
global getCR2

loadPageDirectory:
	push ebp
	mov ebp, esp,
	mov eax, [esp + 8]
	mov cr3, eax
	mov esp, ebp
	pop ebp
	ret
enablePaging:
	push ebp
	mov ebp, esp
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	mov esp, ebp
	pop ebp
	ret
getCR2:
	mov eax, cr2
	ret