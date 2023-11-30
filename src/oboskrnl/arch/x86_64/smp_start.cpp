/*
	arch/x86_64/smp_start.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <x86_64-utils/asm.h>

#include <arch/x86_64/irq/irq.h>
#include <arch/x86_64/interrupt.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <multitasking/arch.h>
#include <multitasking/cpu_local.h>

#include <limine.h>

#define GS_BASE 0xC0000101
#define CPUID_FSGSBASE (1)
#define CR4_FSGSBASE ((uintptr_t)1<<16)

extern "C" void fpuInit();
extern "C" char _strampoline;
extern "C" char _etrampoline;
extern "C" void loadGDT(uint64_t);
extern "C" struct {
	uint16_t limit; uint64_t base;
} __attribute__((packed)) GDT_Ptr;

namespace obos
{
	extern void InitializeGDTASM();
	extern void InitializeIDT_CPU();
	extern void RegisterExceptionHandlers();
	extern void initSSE();
	extern uint8_t g_lapicIDs[256];
	extern uint8_t g_nCores;
	extern bool g_halt;
	extern void SetIST(void* rsp);
	namespace thread
	{
		static memory::PageMap* kernel_cr3;
		cpu_local* g_cpuInfo = nullptr;
		size_t g_nCPUs = 0;
		static void InitializeGDTCpu(cpu_local* info)
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
			info->arch_specific.gdt[0] = 0;
			info->arch_specific.gdt[1] = 0x00209A0000000000;
			info->arch_specific.gdt[2] = 0x0000920000000000;
			info->arch_specific.gdt[3] = 0x00affb000000ffff;
			info->arch_specific.gdt[4] = 0x00aff3000000ffff;
			gdtEntry* tss = (gdtEntry*)&info->arch_specific.gdt[5];
			uintptr_t base = (uintptr_t)&info->arch_specific.tss;
			tss->access = 0x89;
			tss->granularity = 0x40;
			tss->limitLow = sizeof(cpu_local_arch::tssEntry) - 1;
			tss->baseLow = base & 0xFFFF;
			tss->baseMiddle1 = (base >> 16) & 0xFF;
			tss->baseMiddle2 = (base >> 24) & 0xFF;
			tss->baseHigh = base >> 32;
			info->arch_specific.tss.iopb = 103;
			info->arch_specific.gdtPtr.base = (uint64_t)&info->arch_specific.gdt;
			info->arch_specific.gdtPtr.limit = sizeof(info->arch_specific.gdt) - 1;
			SetIST((byte*)info->temp_stack.addr + info->temp_stack.size);
			loadGDT((uint64_t)&info->arch_specific.gdtPtr);
		}
		[[noreturn]] void ProcessorInit(cpu_local* info)
		{
			kernel_cr3->switchToThis();
			InitializeGDTCpu(info);
			InitializeIDT_CPU();
			wrmsr(GS_BASE, (uintptr_t)info);
			InitializeIrq(false);
			fpuInit();
			initSSE();
			uint32_t unused = 0, rbx = 0;
			__cpuid__(0x7, 0, &unused, &rbx, &unused, &unused);
			if (rbx & CPUID_FSGSBASE)
				setCR4(getCR4() & ~CR4_FSGSBASE);
			setupTimerInterrupt();
			atomic_set(&info->initialized);
			infiniteHLT();
		}
		bool StartCPUs()
		{
			g_nCPUs = g_nCores;
			g_cpuInfo = new cpu_local[g_nCPUs];
			kernel_cr3 = memory::getCurrentPageMap();
			memory::MapVirtualPageToPhysical(nullptr, nullptr, 3);
			(*(void**)0xFC8) = (void*)(uintptr_t)*((uint16_t*)&GDT_Ptr);
			(*(void**)0xFF0) = *((void**)((&GDT_Ptr) + 2));
			(*(void**)0xFF8) = kernel_cr3;
			(*(void**)0xFA8) = (void*)ProcessorInit;
			utils::memcpy(memory::mapPageTable(nullptr), &_strampoline, ((uintptr_t)&_etrampoline - (uintptr_t)&_strampoline)-0x58);
			uintptr_t flags = saveFlagsAndCLI();
			memory::VirtualAlloc((void*)0xFFFFFFFF90010000, (g_nCPUs - 1) * 4, memory::PROT_NO_COW_ON_ALLOCATE);
			uintptr_t temp_stacks_base = (0xFFFFFFFF90010000 + ((g_nCPUs - 1) * 4) * 4096);
			memory::VirtualAlloc((void*)temp_stacks_base, g_nCPUs * 4, memory::PROT_NO_COW_ON_ALLOCATE);
			for (size_t i = 0; i < g_nCPUs; i++)
			{
				if (g_lapicIDs[i] == g_localAPICAddr->lapicID)
				{
					InitializeGDTCpu(&g_cpuInfo[i]);
					g_cpuInfo[i].temp_stack.addr = (void*)temp_stacks_base;
					g_cpuInfo[i].temp_stack.size = 0x4000;
					continue;
				}
				(*(void**)0xFD8) = &g_cpuInfo[i];
				if (i != 1)
					g_cpuInfo[i].startup_stack.addr = (char*)g_cpuInfo[i - 1].startup_stack.addr + 0x4000;
				else
					g_cpuInfo[i].startup_stack.addr = (void*)0xFFFFFFFF90008000;
				g_cpuInfo[i].temp_stack.addr = (char*)g_cpuInfo[i - 1].temp_stack.addr + 0x4000;
				g_cpuInfo[i].cpuId = i;
				g_cpuInfo[i].startup_stack.size = 0x4000;
				g_cpuInfo[i].temp_stack.size = 0x4000;
				// Assert the INIT# pin of the AP.
				g_localAPICAddr->interruptCommand32_63 = g_lapicIDs[i] << (56 - 32);
				g_localAPICAddr->interruptCommand0_31 = 0xC500;
				// Wait for the IPI to finish being sent
				while (g_localAPICAddr->interruptCommand0_31 & (1 << 12))
					pause();
				// Send a Startup IPI to the AP.
				g_localAPICAddr->interruptCommand32_63 = g_lapicIDs[i] << (56 - 32);
				g_localAPICAddr->interruptCommand0_31 = 0x600;
				// Wait for the IPI to finish being sent
				while (g_localAPICAddr->interruptCommand0_31 & (1 << 12))
					pause();
				while (!(*(uintptr_t*)0xfe8)); // Wait for "trampoline_done_jumping"
				(*(uintptr_t*)0xfe8) = false;
			}
			g_cpuInfo[0].initialized = true;
			bool allInitialized = false;
			while (!allInitialized)
			{
				allInitialized = true;
				for (size_t i = 1; i < g_nCPUs; i++)
				{
					allInitialized = allInitialized && g_cpuInfo[i].initialized;
					if (!allInitialized)
						break;
				}
			}
			memory::MapVirtualPageToPhysical(nullptr, nullptr, 0);
			utils::memzero(memory::mapPageTable(nullptr), 0x1000);
			restorePreviousInterruptStatus(flags);
			wrmsr(GS_BASE, (uintptr_t)g_cpuInfo);
			return true;
		}
		void StopCPUs(bool includingSelf)
		{
			g_halt = true; // tell the nmi handler to halt the cpu.
			g_localAPICAddr->interruptCommand32_63 = 0;
			// Send an NMI to all other cpus.
			g_localAPICAddr->interruptCommand0_31 = 0xC0400;
			
			while (includingSelf);
		}
	}
}