/*
	oboskrnl/memory_manager/x86-64/init.cpp
	
	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/init.h>
#include <memory_manager/paging/allocate.h>

#include <memory_manager/physical.h>

#include <new>

#include <utils/memory.h>

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
			return reinterpret_cast<UINTPTR_T*>(0xFFFFF000 + level3PageMap * 8);
		}
		UINTPTR_T* PageMap::getLevel3PageMap(UINT16_T level3PageMap)
		{
			kmap_pageTable(m_array);
			return reinterpret_cast<UINTPTR_T*>(*reinterpret_cast<UINTPTR_T*>(0xFFFFF000 + level3PageMap * sizeof(UINTPTR_T)) & 0xFFFFFFFFFF000);
		}
		UINTPTR_T *PageMap::getPageDirectory(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex)
        {
            return reinterpret_cast<UINTPTR_T*>(kmap_pageTable(getLevel3PageMap(level3PageMap))[pageDirectoryIndex] & 0xFFFFFFFFFF000);
        }
        UINTPTR_T *PageMap::getPageDirectoryRealtive(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex)
        {
            return kmap_pageTable(getLevel3PageMap(level3PageMap)) + pageDirectoryIndex;
        }
        UINTPTR_T *PageMap::getPageTable(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex)
        {
            return reinterpret_cast<UINTPTR_T*>(kmap_pageTable(getPageDirectory(level3PageMap, pageDirectoryIndex))[pageTableIndex] & 0xFFFFFFFFFF000);
        }
        UINTPTR_T *PageMap::getPageTableRealtive(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex)
        {
            return kmap_pageTable(getPageDirectory(level3PageMap, pageDirectoryIndex)) + pageTableIndex;
        }
        UINTPTR_T PageMap::getPageTableEntry(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex, UINT16_T index)
        {
            return kmap_pageTable(getPageTable(level3PageMap, pageDirectoryIndex, pageTableIndex))[index] & 0xFFFFFFFFFF000;
        }
		UINTPTR_T PageMap::getRealPageTableEntry(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex, UINT16_T index)
		{
			return kmap_pageTable(getPageTable(level3PageMap, pageDirectoryIndex, pageTableIndex))[index];
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
		UINTPTR_T* g_zeroPage = nullptr;

		void InitializePaging()
		{
			g_level4PageMap = &g_kernelPageMap;
			new (g_level4PageMap) PageMap{ &boot_page_level4_map };
			g_zeroPage = reinterpret_cast<UINTPTR_T*>(kalloc_physicalPage());
			utils::dwMemset((DWORD*)kmap_pageTable(g_zeroPage), 0, 4096 / 4);
		}
	}
}