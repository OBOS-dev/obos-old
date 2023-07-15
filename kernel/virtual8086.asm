;	virual8086.asm
;
;	Copyright (c) 2023 Omar Berrow

; void enter_v8086(uint32_t ss, uint32_t esp, uint32_t cs, uint32_t eip);
global enter_v8086

enter_v8086:
	mov ebp, esp
	push dword  [ebp+4]
	push dword  [ebp+8]

	pushfd
	or dword [esp], (1 << 17)
	push dword [ebp+12]
	push dword  [ebp+16]
	iret