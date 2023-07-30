/* 
	gdt.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

namespace obos
{
	// A gdt in the entry.
	class GdtEntry
	{
	public:
		GdtEntry() = default;
		GdtEntry(UINT32_T base, UINT32_T limit, UINT8_T access, UINT8_T gran);
	private:
		UINT16_T m_limitLow;           // The lower 16 bits of the limit.
		UINT16_T m_baseLow;            // The lower 16 bits of the base.
		UINT8_T  m_baseMiddle;         // The next 8 bits of the base.
		UINT8_T  m_access;              // Access flags, determine what ring this segment can be used in.
		UINT8_T  m_granularity;
		UINT8_T  m_baseHigh;
	};

	void InitializeGdt();
}