/*
	arch/x86_64/memory_manager/virtual/initialize.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <memory_manipulation.h>

#include <limine.h>

#include <x86_64-utils/asm.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/irq/irq.h>

#include <multitasking/cpu_local.h>

#include <allocators/vmm/vmm.h>

namespace obos
{
	namespace memory
	{
		bool g_initialized = false;

		static uintptr_t s_pageDirectory[512] alignas(4096);
		static uintptr_t s_pageTable[512] alignas(4096);
		uintptr_t s_pageTablePhys = 0;
		uintptr_t s_pageDirectoryPhys = 0;
		uintptr_t DecodeProtectionFlags(uintptr_t _flags);

		volatile limine_hhdm_request hhdm_offset = {
			.id = LIMINE_HHDM_REQUEST,
			.revision = 0,
		};

		uintptr_t* mapPageTable(uintptr_t* phys)
		{
			/*if(!g_initialized)
				return phys;*/
			return (uintptr_t*)((byte*)phys + hhdm_offset.response->offset);
			/*uintptr_t ret = 0xfffffffffffff000;
			auto cpu_local_ptr = thread::GetCurrentCpuLocalPtr();
			if (cpu_local_ptr)
				ret = cpu_local_ptr->arch_specific.mapPageTableBase;
			phys = reinterpret_cast<uintptr_t*>((uintptr_t)phys & (~0xfff));
			uintptr_t flags = DecodeProtectionFlags(0) | 1;
			if (!s_pageDirectory[PageMap::addressToIndex(ret, 1)])
				s_pageDirectory[PageMap::addressToIndex(ret, 1)] = (uintptr_t)s_pageTablePhys | flags;
			s_pageTable[PageMap::addressToIndex(ret, 0)] = 0;
			invlpg(ret);
			s_pageTable[PageMap::addressToIndex(ret, 0)] = (uintptr_t)phys | flags;
			invlpg(ret);
			return (uintptr_t*)ret;*/
		}

		uintptr_t* PageMap::getL4PageMapEntryAt(uintptr_t at)
		{
			uintptr_t flags = saveFlagsAndCLI();
			at &= ~0xfff;
			uintptr_t* pageMap = mapPageTable(getPageMap());
			uintptr_t* ret = (uintptr_t*)pageMap[addressToIndex(at, 3)];
			restorePreviousInterruptStatus(flags);
			return ret;
		}
		uintptr_t* PageMap::getL3PageMapEntryAt(uintptr_t at)
		{
			at &= ~0xfff;
			uintptr_t rawPtr = (uintptr_t)getL4PageMapEntryAt(at) & 0xFFFFFFFFFF000;
			if (!rawPtr)
				return nullptr;
			uintptr_t flags = saveFlagsAndCLI();
			uintptr_t* pageMap = mapPageTable((uintptr_t*)rawPtr);
			uintptr_t* ret = (uintptr_t*)pageMap[addressToIndex(at, 2)];
			restorePreviousInterruptStatus(flags);
			return ret;
		}
		uintptr_t* PageMap::getL2PageMapEntryAt(uintptr_t at) 
		{
			at &= ~0xfff;
			uintptr_t flags = saveFlagsAndCLI();
			uintptr_t rawPtr = (uintptr_t)getL3PageMapEntryAt(at) & 0xFFFFFFFFFF000;
			if (!rawPtr)
				return nullptr;
			uintptr_t* pageMap = mapPageTable((uintptr_t*)rawPtr);
			uintptr_t *ret = (uintptr_t*)pageMap[addressToIndex(at, 1)];
			restorePreviousInterruptStatus(flags);
			return ret;
		}
		uintptr_t* PageMap::getL1PageMapEntryAt(uintptr_t at) 
		{
			at &= ~0xfff;
			uintptr_t flags = saveFlagsAndCLI();
			uintptr_t rawPtr = (uintptr_t)getL2PageMapEntryAt(at) & 0xFFFFFFFFFF000;
			if (!rawPtr)
				return nullptr;
			uintptr_t* pageMap = mapPageTable((uintptr_t*)rawPtr);
			uintptr_t* ret = (uintptr_t*)pageMap[addressToIndex(at, 0)];
			restorePreviousInterruptStatus(flags);
			return ret;
		}

		void* MapPhysicalAddress(PageMap* pageMap, uintptr_t phys, void* to, uintptr_t cpuFlags);
		void InitializeVirtualMemoryManager()
		{
			PageMap* kernelPageMap = getCurrentPageMap();
			PageMap* newKernelPageMap = (PageMap*)allocatePhysicalPage();
			uintptr_t* pPageMap = (uintptr_t*)newKernelPageMap;
			s_pageTablePhys = (uintptr_t)kernelPageMap->getL1PageMapEntryAt((uintptr_t)&s_pageTable) & 0xFFFFFFFFFF000;
			s_pageDirectoryPhys = (uintptr_t)kernelPageMap->getL1PageMapEntryAt((uintptr_t)&s_pageDirectory) & 0xFFFFFFFFFF000;
			utils::memzero(newKernelPageMap, 4096);
			pPageMap[511] = (uintptr_t)kernelPageMap->getL4PageMapEntryAt(0xffffffff80000000);
			pPageMap[PageMap::addressToIndex(0xffff800000000000, 3)] = (uintptr_t)kernelPageMap->getL4PageMapEntryAt(0xffff800000000000);
			reinterpret_cast<uintptr_t*>((uintptr_t)newKernelPageMap->getL4PageMapEntryAt(0xfffffffffffff000) & 0xFFFFFFFFFF000)
				[PageMap::addressToIndex(0xfffffffffffff000, 2)] = s_pageDirectoryPhys | 3;
			newKernelPageMap->switchToThis();
			g_initialized = true;
			utils::memzero(mapPageTable(nullptr), 4096);
			freePhysicalPage((uintptr_t)kernelPageMap);
			MapPhysicalAddress(newKernelPageMap, (uintptr_t)g_localAPICAddr, (void*)0xffffffffffffe000, DecodeProtectionFlags(PROT_DISABLE_CACHE | PROT_IS_PRESENT));
			// Do it twice. Why, I don't know. But that fixes the problem
			MapPhysicalAddress(newKernelPageMap, (uintptr_t)g_localAPICAddr, (void*)0xffffffffffffe000, DecodeProtectionFlags(PROT_DISABLE_CACHE | PROT_IS_PRESENT));
			MapPhysicalAddress(newKernelPageMap, (uintptr_t)g_HPETAddr, (void*)0xffffffffffffd000, DecodeProtectionFlags(PROT_DISABLE_CACHE | PROT_IS_PRESENT));
			g_localAPICAddr = (volatile LAPIC*)0xffffffffffffe000;
			g_HPETAddr = (volatile HPET*)0xffffffffffffd000;
			g_initialized = true;
		}
		bool CPUSupportsExecuteDisable()
		{
			return getEFER() & ((uintptr_t)1 << 11);
		}
	}
}