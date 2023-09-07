/*
	oboskrnl/descriptors/idt/x86_64/idt.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

namespace obos
{
	class IdtEntry
	{
	public:
		IdtEntry() = default;
		IdtEntry(UINTPTR_T base, UINT16_T sel, UINT8_T typeAttributes, UINT8_T ist);
	private:
		UINT16_T m_offset1;
		UINT16_T m_selector;
		UINT8_T m_ist;
		UINT8_T m_typeAttributes;
		UINT16_T m_offset2;
		UINT32_T m_offset3;
		UINT32_T m_resv1;
	};

	struct interrupt_frame
	{
		// (ds) 0
		UINT64_T ds;
		// (r8-r15) 8,16,24,32,40,48,56,64
		// (rdi-rax) 72,80,88,96,104,112,120,128
		UINTPTR_T r8, r9, r10, r11, r12, r13, r14, r15, 
				  rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
		// (intNumber) 136
		UINT8_T intNumber;
		// (errorCode) 144
		UINT64_T errorCode;
		// (rip,cs,rflags,useresp,ss)
		// 152,160,168,176,184
		UINTPTR_T rip, cs, rflags, useresp, ss;
		// 192
	};

	void InitializeIdt();
	void RegisterInterruptHandler(UINT8_T interrupt, void(*isr)(const interrupt_frame* frame));
}