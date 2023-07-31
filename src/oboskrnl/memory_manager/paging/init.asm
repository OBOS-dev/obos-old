[BITS 32]

[global _ZN4obos6memory13PageDirectory12switchToThisEv]
[global _ZN4obos6memory19GetPageFaultAddressEv]

; obos::memory::PageDirectory::switchToThis()
_ZN4obos6memory13PageDirectory12switchToThisEv:
	push ebp
	mov ebp, esp,
	mov eax, [esp + 8]
	mov cr3, eax
	mov esp, ebp
	pop ebp
	push ebp
	mov ebp, esp
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	mov esp, ebp
	leave
	ret
; obos::memory::GetPageFaultAddress()
_ZN4obos6memory19GetPageFaultAddressEv:
	mov eax, cr2
	ret