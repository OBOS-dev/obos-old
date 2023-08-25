/*
	oboskrnl/descriptors/gdt/gdt.cpp

	Copyright (c) 2023 Omar Berrow
*/

#ifdef __i686__
#include <descriptors/gdt/gdt.h>

#include <new>

extern "C" void gdtFlush(UINTPTR_T base);
extern "C" void tssFlush();

namespace obos
{
	struct TssEntry
	{
		UINT32_T prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
		UINT32_T esp0;     // The stack pointer to load when changing to kernel mode.
		UINT32_T ss0;      // The stack segment to load when changing to kernel mode.
		// Everything below here is unused.
		UINT32_T esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
		UINT32_T ss1;
		UINT32_T esp2;
		UINT32_T ss2;
		UINT32_T cr3;
		UINT32_T eip;
		UINT32_T eflags;
		UINT32_T eax;
		UINT32_T ecx;
		UINT32_T edx;
		UINT32_T ebx;
		UINT32_T esp;
		UINT32_T ebp;
		UINT32_T esi;
		UINT32_T edi;
		UINT32_T es;
		UINT32_T cs;
		UINT32_T ss;
		UINT32_T ds;
		UINT32_T fs;
		UINT32_T gs;
		UINT32_T ldt;
		UINT16_T trap;
		UINT16_T iomap_base;
	} attribute(packed);

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
	static GdtEntry s_gdtEntries[6];
	TssEntry g_tssEntry;

	void InitializeGdt()
	{
		new (s_gdtEntries + 0) GdtEntry(0, 0, 0, 0);
		new (s_gdtEntries + 1) GdtEntry(0, 0xFFFFFFFF, 0x9A, 0xCF);
		new (s_gdtEntries + 2) GdtEntry(0, 0xFFFFFFFF, 0x92, 0xCF);
		new (s_gdtEntries + 3) GdtEntry(0, 0xFFFFFFFF, 0xFA, 0xCF);
		new (s_gdtEntries + 4) GdtEntry(0, 0xFFFFFFFF, 0xF2, 0xCF);

		new (s_gdtEntries + 5) GdtEntry((UINTPTR_T)&g_tssEntry, (UINTPTR_T)&g_tssEntry + sizeof(g_tssEntry), 0xE9, 0x00);
	
		g_tssEntry.ss0 = 0x10;
		g_tssEntry.esp0 = 0x00;
		g_tssEntry.cs = (3 * 8) | 3;
		g_tssEntry.ss = g_tssEntry.ds = g_tssEntry.es = g_tssEntry.fs = g_tssEntry.gs = (4 * 8) | 3;

		s_gdtPointer.limit = sizeof(s_gdtEntries) - 1;
		s_gdtPointer.base  = (UINTPTR_T)&s_gdtEntries;

		gdtFlush((UINTPTR_T)&s_gdtPointer);
		tssFlush();
	}
	void SetTSSStack(PVOID esp0)
	{
		g_tssEntry.esp0 = (UINTPTR_T)esp0;
	}
}
#else
namespace obos
{
	void InitializeGdt()
	{
		// If we're not on i686, then do nothing.
	}
	void SetTssStack(PVOID) {}
}
#endif