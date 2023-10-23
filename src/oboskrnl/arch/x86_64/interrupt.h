/*
	oboskrnl/arch/x86_64/interrupt_frame.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#define BITFIELD_FROM_BIT(n) (1<<n)

namespace obos
{
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
		explicit operator uintptr_t() { return m_bitfield; }
		x86_64_flags() = default;
		x86_64_flags(uintptr_t flags)
		{
			m_bitfield = flags | 2;
		}
		void setBit(uintptr_t bitmask)
		{
			bitmask |= 0b10;
			m_bitfield |= bitmask;
		}
		void clearBit(uintptr_t bitmask)
		{
			bitmask &= ~(0b10);
			m_bitfield &= (~bitmask);
		}
		bool getBit(uintptr_t bitmask) const
		{
			return m_bitfield & bitmask;
		}

		bool operator[](uintptr_t bitmask) const
		{
			return getBit(bitmask);
		}
		uintptr_t m_bitfield;
	};

	struct interrupt_frame
	{
		// (ss) 0
		uintptr_t ss;
		// (r8-r15) 8,16,24,32,40,48,56,64
		// (rdi-rax) 72,80,88,96,104,112,120,128
		uintptr_t rbp, r8, r9, r10, r11, r12, r13, r14, r15,
				 rdi, rsi, ignored, rbx, rdx, rcx, rax;
		// (intNumber) 136, (errorCode) 144
		uintptr_t intNumber, errorCode;
		// (rip,cs,rflags,useresp,ds)
		// 152,160,168,176,184
		uintptr_t rip, cs;
		x86_64_flags rflags;
		uintptr_t rsp, ds;
	};

	void RegisterInterruptHandler(byte interrupt, void(*handler)(interrupt_frame* frame));
}