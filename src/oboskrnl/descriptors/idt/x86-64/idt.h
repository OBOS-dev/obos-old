/*
	oboskrnl/descriptors/idt/x86_64/idt.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#include <utils/bitfields.h>

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

	struct x86_64_flags
	{
		enum
		{
			RFLAGS_CARRY = BITFIELD_FROM_BIT(0),
			RFLAGS_PARITY = BITFIELD_FROM_BIT(2),
			RFLAGS_AUXILLARY_CARRY = BITFIELD_FROM_BIT(4),
			RFLAGS_ZERO = BITFIELD_FROM_BIT(6),
			RFLAGS_SIGN = BITFIELD_FROM_BIT(7),
			RFLAGS_TRAP = BITFIELD_FROM_BIT(8),
			RFLAGS_INTERRUPT_ENABLE = BITFIELD_FROM_BIT(9),
			RFLAGS_DIRECTION = BITFIELD_FROM_BIT(10),
			RFLAGS_OVERFLOW = BITFIELD_FROM_BIT(11),
			RFLAGS_NESTED_TASK = BITFIELD_FROM_BIT(14),
			RFLAGS_RESUME = BITFIELD_FROM_BIT(16),
			RFLAGS_VIRTUAL8086 = BITFIELD_FROM_BIT(17),
			RFLAGS_ALIGN_CHECK = BITFIELD_FROM_BIT(18),
			RFLAGS_VINTERRUPT_FLAG = BITFIELD_FROM_BIT(19),
			RFLAGS_VINTERRUPT_PENDING = BITFIELD_FROM_BIT(20),
			RFLAGS_CPUID = BITFIELD_FROM_BIT(21),
		};
		explicit operator UINTPTR_T() { return m_bitfield; }
		x86_64_flags() = default;
		x86_64_flags(UINTPTR_T flags)
		{
			m_bitfield = flags;
		}
		void setBit(UINTPTR_T bitmask)
		{
			m_bitfield |= bitmask;
		}
		void clearBit(UINTPTR_T bitmask)
		{
			m_bitfield &= (~bitmask);
		}
		bool getBit(UINTPTR_T bitmask) const
		{
			return m_bitfield & bitmask;
		}

		bool operator[](UINTPTR_T bitmask) const
		{
			return getBit(bitmask);
		}
		UINTPTR_T m_bitfield;
	};

	struct interrupt_frame
	{
		// (ds) 0
		UINTPTR_T ds;
		// (r8-r15) 8,16,24,32,40,48,56,64
		// (rdi-rax) 72,80,88,96,104,112,120,128
		UINTPTR_T r8, r9, r10, r11, r12, r13, r14, r15,
				  rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
		// (intNumber) 136
		UINTPTR_T intNumber;
		// (errorCode) 144
		UINTPTR_T errorCode;
		// (rip,cs,rflags,useresp,ss)
		// 152,160,168,176,184
		UINTPTR_T rip, cs;
		x86_64_flags rflags;
		UINTPTR_T useresp, ss;
		// 192
	};

	void InitializeIdt();
	void RegisterInterruptHandler(UINT8_T interrupt, void(*isr)(const interrupt_frame* frame));
}