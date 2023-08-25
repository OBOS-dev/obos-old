/*
	oboskrnl/memory_manager/x86-64/init.cpp
	
	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/init.h>
#include <memory_manager/paging/allocate.h>

#include <memory_manager/physical.h>

#include <new>

extern UINTPTR_T boot_page_level4_map;

namespace obos
{
	namespace memory
	{
		extern UINTPTR_T* kmap_pageTable(PVOID physicalAddress);

		PageMap::PageMap()
		{
			m_array = reinterpret_cast<UINTPTR_T*>(kalloc_physicalPage());
			m_owns = true;
			m_initialized = true;
		}
		PageMap::PageMap(UINTPTR_T* array)
			:m_array{array}, m_owns{false}, m_initialized{true}
		{}

		UINTPTR_T* PageMap::getLevel3PageMapAddress(UINT16_T level3PageMap)
		{
			kmap_pageTable(m_array);
			return reinterpret_cast<UINTPTR_T*>(reinterpret_cast<UINTPTR_T*>(0xFFFFF000) + level3PageMap);
		}
		UINTPTR_T* PageMap::getLevel3PageMap(UINT16_T level3PageMap)
		{
			kmap_pageTable(m_array);
			return reinterpret_cast<UINTPTR_T*>(*reinterpret_cast<UINTPTR_T*>(reinterpret_cast<UINTPTR_T*>(0xFFFFF000) + level3PageMap) & 0xFFFFFFFFFE000);
		}

		void PageMap::switchToThis()
		{
			g_level4PageMap = this;
			asm volatile ("mov %0, %%cr3" : :"r"(m_array) : "memory");
		}

		PageMap::~PageMap()
		{
			if (m_owns)
				kfree_physicalPage(m_array);
		}


		PageMap* g_level4PageMap;
		PageMap g_kernelPageMap;

		void InitializePaging()
		{
			g_level4PageMap = &g_kernelPageMap;
			new (g_level4PageMap) PageMap{ &boot_page_level4_map };
		}
	}
}