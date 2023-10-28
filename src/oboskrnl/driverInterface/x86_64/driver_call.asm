[BITS 64]

extern _ZN4obos15driverInterface14g_syscallTableE

; (in) rdi: A pointer to a structure with the parameters.
; (in) rax: The syscall number.
; (out) rax: Lower 64 bits of the return value.
; (out) rdx: Upper 64 bits of the return value.
; Registers are not preserved.
global isr49
isr49:
	cli
	cld

	lea rax, [rax*8+_ZN4obos15driverInterface14g_syscallTableE]
	test rax,rax
	jz .finish
	call [rax]
.finish:
	iretq