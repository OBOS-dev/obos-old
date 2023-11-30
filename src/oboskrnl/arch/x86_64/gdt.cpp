/*
	arch/x86_64/gdt.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <stdint.h>

#include <memory_manipulation.h>

#include <multitasking/cpu_local.h>
#include <multitasking/arch.h>

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
		uint64_t rsp1;
		uint64_t rsp2;
		uint64_t resv2;
		uint64_t ist0;
		uint8_t unused1[0x3a];
		uint16_t iopb;
	} __attribute__((packed));

	extern "C" gdtEntry TSS[sizeof(gdtEntry)];
	
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
		static char ist0_tstack[0x1800];
		s_tssEntry.ist0 = (uint64_t)(ist0_tstack + 0x1800);
		
		InitializeGDTASM();
	}
	void SetTSSStack(void* rsp)
	{
		if (thread::GetCurrentCpuLocalPtr())
			thread::GetCurrentCpuLocalPtr()->arch_specific.tss.rsp0 = (uintptr_t)rsp;
		else
			s_tssEntry.rsp0 = (uintptr_t)rsp;
	}
	void SetIST(void* rsp)
	{
		if(thread::GetCurrentCpuLocalPtr())
			thread::GetCurrentCpuLocalPtr()->arch_specific.tss.ist0 = (uint64_t)rsp;
		else
			s_tssEntry.ist0 = (uintptr_t)rsp;
	}
}
