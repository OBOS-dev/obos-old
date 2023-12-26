[BITS 64]

extern _ZN4obos15driverInterface14g_syscallTableE

; (in) rdi: A pointer to a structure with the parameters.
; (in) rax: The syscall number.
; (out) rax: Lower 64 bits of the return value.
; (out) rdx: Upper 64 bits of the return value.
; Registers are not explicitly preserved, but you can expect them to be preserved in the way the SystemV ABI preserves them (non-guarentee).
global isr49
isr49:
	cld

	lea rax, [rax*8+_ZN4obos15driverInterface14g_syscallTableE]
	test rax,rax
	jz .finish

	push r15
	mov r15, cr4
	test r15, (1<<21) ; cr4.SMAP[bit 21]
	jz .call
	stac
.call:
	pop r15

	call [rax]
.finish:
	iretq