/*
	drivers/x86_64/sata/command.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <x86_64-utils/asm.h>

#include "command.h"
#include "structs.h"

extern volatile HBA_MEM* g_generalHostControl;

asm(
".global _Z17StopCommandEnginePV8HBA_PORT;"
".global _Z18StartCommandEnginePV8HBA_PORT;"
".global _Z11FindCMDSlotPV8HBA_PORT;"
".intel_syntax noprefix;"
"_Z11FindCMDSlotPV8HBA_PORT:;"
"	mov eax, dword ptr [rdi+0x34];"
"	or eax, [rdi+0x38];"
"	mov esi, eax;"
"	not esi;"
"	bsf eax, esi;"
"	ret;"
"_Z17StopCommandEnginePV8HBA_PORT:;"
"	push rbp;"
"	mov rbp, rsp;"
"	and dword ptr [rdi+0x18], ~(1<<0);" // Clear ST (bit 0)
"	and dword ptr [rdi+0x18], ~(1<<4);" // Clear FRE (bit 4)
// Wait until FR (bit 14), CR (bit 15) are cleared
"_Z17StopCommandEnginePV8HBA_PORT.loop:;"
"	test dword ptr [rdi+0x18], (1<<14);"
"	jnz _Z17StopCommandEnginePV8HBA_PORT.loop;"
"	test dword ptr [rdi+0x18], (1<<15);"
"	jnz _Z17StopCommandEnginePV8HBA_PORT.loop;"
"_Z17StopCommandEnginePV8HBA_PORT.end:;"
"	leave;"
"	ret;"
".intel_syntax noprefix;"
"_Z18StartCommandEnginePV8HBA_PORT:;"
"	push rbp;"
"	mov rbp, rsp;"
"_Z18StartCommandEnginePV8HBA_PORT.loop:;"
"	test dword ptr [rdi+0x18], (1<<15);"
"	jnz _Z18StartCommandEnginePV8HBA_PORT.loop;"
"_Z18StartCommandEnginePV8HBA_PORT.end:;"
"	or dword ptr [rdi+0x18], (1<<4);"
"	or dword ptr [rdi+0x18], (1<<0);"
"	leave;"
"	ret;"
".att_syntax prefix;"
);