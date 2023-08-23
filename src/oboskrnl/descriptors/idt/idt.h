/*
	idt.h

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
		IdtEntry(UINT32_T base, UINT16_T sel, UINT8_T flags);
	private:
		UINT16_T m_baseLow;
		UINT16_T m_sel;
		UINT8_T  m_always0;
		UINT8_T  m_flags;
		UINT16_T m_baseHigh;
	};

	struct interrupt_frame
	{
		// +0		
		UINT32_T ds;
		//		  +4,  +8,  +12, +16, +20, +24, +28, +32
		UINTPTR_T edi, esi, ebp, esp, ebx, edx, ecx, eax;
		// +36
		UINT8_T intNumber;
		// +40 (Padding)
		UINT32_T errorCode;
		//		 +44,+48,    +52,	   +56,+60
		UINTPTR_T eip, cs , eflags, useresp, ss;
		// +64 (End)
	};

	void InitializeIdt();
	void RegisterInterruptHandler(UINT8_T interrupt, void(*isr)(const interrupt_frame* frame));
}