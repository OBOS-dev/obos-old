/*
	gdt.cpp

	Copyright (c) 2023 gdt.cpp
*/
#include <descriptors/gdt/gdt.h>

extern "C" void gdtFlush(UINTPTR_T base);

namespace obos
{
	GdtEntry::GdtEntry(UINT32_T base, UINT32_T limit, UINT8_T access, UINT8_T gran)
	{
		m_baseLow = (base & 0xFFFF);
		m_baseMiddle = (base >> 16) & 0xFF;
		m_baseHigh = (base >> 24) & 0xFF;

		m_limitLow = (limit & 0xFFFF);
		m_granularity = (limit >> 16) & 0x0F;

		m_granularity |= gran & 0xF0;
		m_access = access;
	}

	struct GdtPointer
	{
		UINT16_T limit;
		UINTPTR_T base;
	} attribute(packed);


	static GdtPointer s_gdtPointer;
	static GdtEntry s_gdtEntries[5];

	void InitializeGdt()
	{
		s_gdtEntries[0] = GdtEntry(0, 0, 0, 0);
		s_gdtEntries[1] = GdtEntry(0, 0xFFFFFFFF, 0x9A, 0xCF);
		s_gdtEntries[2] = GdtEntry(0, 0xFFFFFFFF, 0x92, 0xCF);
		s_gdtEntries[3] = GdtEntry(0, 0xFFFFFFFF, 0xFA, 0xCF);
		s_gdtEntries[4] = GdtEntry(0, 0xFFFFFFFF, 0xF2, 0xCF);
	
		s_gdtPointer.limit = sizeof(s_gdtEntries) - 1;
		s_gdtPointer.base  = (UINTPTR_T)&s_gdtEntries;

		gdtFlush((UINTPTR_T)&s_gdtPointer);
	}
}
