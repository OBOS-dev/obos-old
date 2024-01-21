/*
	oboskrnl/multitasking/x86_64/taskSwitchInfo.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/interrupt.h>

namespace obos
{
	namespace thread
	{
		struct taskSwitchInfo
		{
			void* cr3;
			void* tssStackBottom;
			interrupt_frame frame;
			alignas(0x10) uint8_t fpuState[512];
			void* syscallStackBottom;
		};
		struct cpu_local_arch
		{
			struct tssEntry
			{
				uint32_t resv1;
				uint64_t rsp0;
				uint64_t rsp1;
				uint64_t rsp2;
				uint64_t resv2;
				uint64_t ist0;
				uint8_t unused1[0x36];
				uint16_t iopb;
			} __attribute__((packed));
			uint64_t gdt[7];
			tssEntry tss;
			struct gdtptr
			{
				uint16_t limit;
				uint64_t base;
			} __attribute__((packed));
			gdtptr gdtPtr;
			uintptr_t mapPageTableBase;
		};
	}
}