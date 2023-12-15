/*
	oboskrnl/process/x86_64/procInfo.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <multitasking/process/arch.h>

#include <multitasking/cpu_local.h>
#include <multitasking/arch.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <memory_manipulation.h>

#include <x86_64-utils/asm.h>

namespace obos
{
	extern void SetIST(void* rsp);
	namespace memory
	{
		extern uintptr_t s_pageDirectoryPhys;
	}
	namespace process
	{
		void setupContextInfo(procContextInfo* info)
		{
			uintptr_t flags = saveFlagsAndCLI();
			memory::PageMap* currentPageMap = memory::getCurrentPageMap();
			uintptr_t entry80000000 = (uintptr_t)currentPageMap->getL4PageMapEntryAt(0xffffffff80000000);
			uintptr_t entryffff80000000_0 = (uintptr_t)currentPageMap->getL4PageMapEntryAt(0xffff800000000000);
			memory::PageMap* newPageMap = (memory::PageMap*)memory::allocatePhysicalPage();
			uintptr_t* pPageMap = (uintptr_t*)newPageMap;
			utils::memzero(memory::mapPageTable(pPageMap), 4096);
			memory::mapPageTable(pPageMap)[511] = entry80000000;
			memory::mapPageTable(pPageMap)[memory::PageMap::addressToIndex(0xffff800000000000, 3)] = entryffff80000000_0;
			memory::mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)newPageMap->getL4PageMapEntryAt(0xfffffffffffff000) & 0xFFFFFFFFFF000))
				[memory::PageMap::addressToIndex(0xfffffffffffff000, 2)] = memory::s_pageDirectoryPhys | 3;
			info->cr3 = newPageMap;
			restorePreviousInterruptStatus(flags);
		}
		void switchToProcessContext(procContextInfo* info)
		{
			memory::PageMap* cr3 = (memory::PageMap*)info->cr3;
			thread::cpu_local* _info = thread::GetCurrentCpuLocalPtr();
			SetIST((byte*)_info->temp_stack.addr + _info->temp_stack.size);
			cr3->switchToThis();
		}
	}
}