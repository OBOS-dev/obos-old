/*
	oboskrnl/descriptors/gdt/gdt.cpp

	Copyright (c) 2023 Omar Berrow
*/

#if defined(__i686__)
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

	GdtEntry::GdtEntry(UINTPTR_T base, UINT32_T limit, UINT8_T access, UINT8_T gran)
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
#elif defined(__x86_64__)
#include <types.h>

#include <descriptors/gdt/gdt.h>

#include <utils/memory.h>

extern "C" char TSS;
extern "C" char GDT;
namespace obos
{
	struct gdtEntry
	{
		UINT16_T limitLow;
		UINT16_T baseLow;
		UINT8_T  baseMiddle1;
		UINT8_T  access;
		UINT8_T  granularity;
		UINT8_T  baseMiddle2;
		UINT64_T baseHigh;
	};

	struct tssEntry
	{
		UINT32_T resv1;
		UINT64_T rsp0;
		BYTE unused[0x5A];
		UINT16_T iopb;
	} attribute(packed);

	tssEntry s_tssEntry alignas(8);

	static_assert(sizeof(tssEntry) == 104, "sizeof(tssEntry) != 104");

	extern void InitializeGDTASM();

	void InitializeGdt()
	{
		utils::memzero(&s_tssEntry, sizeof(tssEntry));

		gdtEntry* tss = (gdtEntry*)&TSS;

		UINTPTR_T base = GET_FUNC_ADDR(&s_tssEntry);
		tss->access = 0x89;
		tss->granularity = 0x40;
		tss->limitLow = sizeof(tssEntry);
		tss->baseLow = base & 0xFFFF;
		tss->baseMiddle1 = (base >> 16) & 0xFF;
		tss->baseMiddle2 = (base >> 24) & 0xFF;
		tss->baseHigh = base >> 32;
		gdtEntry* userCodeSeg = (gdtEntry*)((&GDT) + 0x18);
		gdtEntry* userDataSeg = (gdtEntry*)((&GDT) + 0x20);
		
		s_tssEntry.iopb = 104;

		userCodeSeg->access = 0xFA;
		userCodeSeg->granularity = 0xAF;
		userCodeSeg->limitLow = 0xFFFF;

		userDataSeg->access = 0xF2;
		userDataSeg->granularity = 0xCF;
		userDataSeg->limitLow = 0xFFFF;

		InitializeGDTASM();
	}
	void SetTSSStack(void* rsp)
	{
		s_tssEntry.rsp0 = (UINTPTR_T)rsp;
	}
}
#else
namespace obos
{
	void InitializeGdt()
	{
		// If we're not on x86, then do nothing.
	}
	void SetTSSStack(void*)
	{}
}
#endif