[BITS 64]

segment .data

align 16
GDT:
; Null entry.
	dq 0
; Code segment.
	dq 0x00209A0000000000
; Data segment.
	dq 0x0000920000000000
; User mode code segment
	dq 0
; User mode data segment
	dq 0
align 1
TSS:
	dq 0,0

align 1

GDT_Ptr:
	dw $- GDT - 1
	dq 0

[GLOBAL TSS]
[GLOBAL GDT]

segment .text

[GLOBAL _ZN4obos16InitializeGDTASMEv]

_ZN4obos16InitializeGDTASMEv:
	push rbp
	mov rbp, rsp

	lea rax, [GDT]
	mov [GDT_Ptr+2], rax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	lgdt [GDT_Ptr]
	
	push 0x8
	push .flush_tss
	retfq
.flush_tss:
	mov ax, 0x28
	ltr ax

	leave
	ret