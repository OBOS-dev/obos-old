; oboskrnl/x86_64-utils/asm.asm

; Copyright (c) 2023 Omar Berrow

[BITS 64]

global _ZN4obos4outbEth
global _ZN4obos4outwEtt
global _ZN4obos4outdEtj
global _ZN4obos3inbEt
global _ZN4obos3inwEt
global _ZN4obos3indEt
global _ZN4obos3cliEv
global _ZN4obos3stiEv
global _ZN4obos3hltEv
global _ZN4obos6getCR2Ev

_ZN4obos4outbEth:
	mov dx, di
	mov al, sil
	out dx, al
	ret
_ZN4obos4outwEtt:
	mov dx, di
	mov ax, si
	out dx, ax
	ret
_ZN4obos4outdEtj:
	mov dx, di
	mov eax, esi
	out dx, eax
	ret
_ZN4obos3inbEt:
	mov eax, 0
	mov dx, di
	in al, dx
	ret
_ZN4obos3inwEt:
	mov eax, 0
	mov dx, di
	in ax, dx
	ret
_ZN4obos3indEt:
	mov dx, di
	in eax, dx
	ret
_ZN4obos3cliEv:
	cli
	ret
_ZN4obos3stiEv:
	sti
	ret
_ZN4obos3hltEv:
	sti
	ret
_ZN4obos6getCR2Ev:
	mov rax, cr2
	ret