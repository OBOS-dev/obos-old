/*
	oboskrnl/process/x86_64/procInfo.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <multitasking/process/arch.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <memory_manipulation.h>

namespace obos
{
	namespace memory
	{
		extern uintptr_t s_pageDirectoryPhys;
	}
	namespace process
	{
		void setupContextInfo(procContextInfo* info)
		{
			memory::PageMap* currentPageMap = memory::getCurrentPageMap();
			memory::PageMap* newPageMap = (memory::PageMap*)memory::allocatePhysicalPage();
			uintptr_t* pPageMap = (uintptr_t*)newPageMap;
			utils::memzero(newPageMap, 4096);
			pPageMap[511] = (uintptr_t)currentPageMap->getL4PageMapEntryAt(0xffffffff80000000);
			pPageMap[memory::PageMap::addressToIndex(0xffff800000000000, 3)] = (uintptr_t)currentPageMap->getL4PageMapEntryAt(0xffff800000000000);
			reinterpret_cast<uintptr_t*>((uintptr_t)newPageMap->getL4PageMapEntryAt(0xfffffffffffff000) & 0xFFFFFFFFFF000)
				[memory::PageMap::addressToIndex(0xfffffffffffff000, 2)] = memory::s_pageDirectoryPhys | 3;

			info->cr3 = newPageMap;
		}
		void switchToProcessContext(procContextInfo* info)
		{
			memory::PageMap* cr3 = (memory::PageMap*)info->cr3;
			cr3->switchToThis();
		}
	}
}