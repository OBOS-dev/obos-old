[BITS 32]

[global _ZN4obos6memory13PageDirectory15switchToThisAsmEj]
[global _ZN4obos6memory12enablePagingEv]
[global _ZN4obos6memory19GetPageFaultAddressEv]

; obos::memory::PageDirectory::switchToThisAsm(UINTPTR_T address)
_ZN4obos6memory13PageDirectory15switchToThisAsmEj:
	push ebp
	mov ebp, esp
	
	; Load the page directory.
	mov eax, [esp + 8]
	mov cr3, eax
	
	leave
	ret
; obos::memory::GetPageFaultAddress()
_ZN4obos6memory19GetPageFaultAddressEv:
	mov eax, cr2
	ret
; obos::memory::enablePaging
_ZN4obos6memory12enablePagingEv:
	push ebp
	mov ebp, esp

	; Enable paging.
	mov eax, cr0
	or eax, 0x80000001
	mov cr0, eax

	leave
	ret