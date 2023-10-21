/*
	arch/x86_64/memory_manager/virtual/initialize.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <memory_manipulation.h>

#include <x86_64-utils/asm.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/irq/irq.h>

namespace obos
{
	namespace memory
	{
		bool g_initialized = false;

		static uintptr_t s_pageDirectory[512] alignas(4096);
		static uintptr_t s_pageTable[512] alignas(4096);
		static uintptr_t s_pageTablePhys = 0;
		static uintptr_t s_pageDirectoryPhys = 0;
		
		uintptr_t* mapPageTable(uintptr_t* phys)
		{
			if(!g_initialized)
				return phys;
			constexpr uintptr_t ret = 0xfffffffffffff000;
			phys = reinterpret_cast<uintptr_t*>((uintptr_t)phys & (~0xfff));
			s_pageDirectory[PageMap::addressToIndex(ret, 1)] = 0;
			s_pageTable[PageMap::addressToIndex(ret, 0)] = 0;
			invlpg(ret);
			s_pageTable[PageMap::addressToIndex(ret, 0)] = (uintptr_t)phys | 3;
			s_pageDirectory[PageMap::addressToIndex(ret, 1)] = (uintptr_t)s_pageTablePhys | 3;
			invlpg(ret);
			return (uintptr_t*)ret;
		}

		uintptr_t* PageMap::getL4PageMapEntryAt(uintptr_t at)
		{
			uintptr_t* pageMap = mapPageTable(getPageMap());
			return (uintptr_t*)pageMap[addressToIndex(at, 3)];
		}
		uintptr_t* PageMap::getL3PageMapEntryAt(uintptr_t at)
		{
			uintptr_t* pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)getL4PageMapEntryAt(at) & 0xFFFFFFFFFF000));
			return (uintptr_t*)pageMap[addressToIndex(at, 2)];
		}
		uintptr_t* PageMap::getL2PageMapEntryAt(uintptr_t at) 
		{
			uintptr_t* pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)getL3PageMapEntryAt(at) & 0xFFFFFFFFFF000));
			return (uintptr_t*)pageMap[addressToIndex(at, 1)];
		}
		uintptr_t* PageMap::getL1PageMapEntryAt(uintptr_t at) 
		{
			uintptr_t* pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)getL2PageMapEntryAt(at) & 0xFFFFFFFFFF000));
			return (uintptr_t*)pageMap[addressToIndex(at, 0)];
		}

		void InitializeVirtualMemoryManager()
		{
			PageMap* kernelPageMap = getCurrentPageMap();
			PageMap* newKernelPageMap = (PageMap*)allocatePhysicalPage();
			uintptr_t* pPageMap = (uintptr_t*)newKernelPageMap;
			s_pageTablePhys = (uintptr_t)kernelPageMap->getL1PageMapEntryAt((uintptr_t)&s_pageTable) & 0xFFFFFFFFFF000;
			s_pageDirectoryPhys = (uintptr_t)kernelPageMap->getL1PageMapEntryAt((uintptr_t)&s_pageDirectory) & 0xFFFFFFFFFF000;
			pPageMap[511] = (uintptr_t)kernelPageMap->getL4PageMapEntryAt(0xffffffff80000000);
			pPageMap[PageMap::addressToIndex(0xffff800000000000, 3)] = (uintptr_t)kernelPageMap->getL4PageMapEntryAt(0xffff800000000000);
			reinterpret_cast<uintptr_t*>((uintptr_t)newKernelPageMap->getL4PageMapEntryAt(0xfffffffffffff000) & 0xFFFFFFFFFF000)
				[PageMap::addressToIndex(0xfffffffffffff000, 2)] = s_pageDirectoryPhys | 3;
			newKernelPageMap->switchToThis();
			g_initialized = true;
			utils::memzero((uint32_t*)mapPageTable(nullptr), 4096);
			freePhysicalPage((uintptr_t)kernelPageMap);
			MapVirtualPageToPhysical((void*)0xffffffffffffe000, (void*)g_localAPICAddr, DecodeProtectionFlags(PROT_DISABLE_CACHE | PROT_IS_PRESENT));
			g_localAPICAddr = (volatile LAPIC*)0xffffffffffffe000;
		}
		bool CPUSupportsExecuteDisable()
		{
			return getEFER() & ((uintptr_t)1 << 11);
		}
	}
}