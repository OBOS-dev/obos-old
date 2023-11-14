/*
	arch/x86_64/gdt.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <stdint.h>

#include <memory_manipulation.h>

namespace obos
{
	struct gdtEntry
	{
		uint16_t limitLow;
		uint16_t baseLow;
		uint8_t  baseMiddle1;
		uint8_t  access;
		uint8_t  granularity;
		uint8_t  baseMiddle2;
		uint64_t baseHigh;
	} __attribute__((packed));

	struct tssEntry
	{
		uint32_t resv1;
		uint64_t rsp0;
		uint8_t unused1[0x5A];
		uint16_t iopb;
	} __attribute__((packed));

	extern "C" char TSS[sizeof(gdtEntry)];
	extern "C" char GDT;

	tssEntry s_tssEntry alignas(8);

	static_assert(sizeof(tssEntry) == 104, "sizeof(tssEntry) != 104");

	extern void InitializeGDTASM();

	void InitializeGdt()
	{
		utils::memzero(&s_tssEntry, sizeof(tssEntry));

		gdtEntry* tss = (gdtEntry*)&TSS;

		uintptr_t base = reinterpret_cast<uintptr_t>(&s_tssEntry);
		tss->access = 0x89;
		tss->granularity = 0x40;
		tss->limitLow = sizeof(tssEntry) - 1;
		tss->baseLow = base & 0xFFFF;
		tss->baseMiddle1 = (base >> 16) & 0xFF;
		tss->baseMiddle2 = (base >> 24) & 0xFF;
		tss->baseHigh = base >> 32;
		s_tssEntry.iopb = 103;
		
		gdtEntry* userCodeSeg = (gdtEntry*)((&GDT) + 0x18);
		gdtEntry* userDataSeg = (gdtEntry*)((&GDT) + 0x20);

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
		s_tssEntry.rsp0 = (uintptr_t)rsp;
	}
}
