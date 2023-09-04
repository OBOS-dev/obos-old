/*
	oboskrnl/memory_manager/paging/init.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

namespace obos
{
	namespace memory
	{
		extern bool CPUSupportsExecuteDisable();
		// Only implemented on i686
		class PageDirectory final
		{
		public:
			PageDirectory();
			PageDirectory(UINTPTR_T* array);

			void init();

			void switchToThis();

			// Always check if the address of the return value of this function is not nullptr.
			UINTPTR_T* getPageTableAddress(UINT16_T pageTable);
			// Always check if the address of the return value of this function is not nullptr.
			UINTPTR_T* getPageTable(UINT16_T pageTable);

			UINTPTR_T* getPageDirectory() { return m_array; }

			~PageDirectory();

			static UINT16_T addressToIndex(UINTPTR_T base);
			static UINT16_T addressToPageTableIndex(UINTPTR_T base);
		private:
			// NOTE: Do not change the position of this variable. Don't do it if you don't want to have a bad time.
			UINTPTR_T* m_array = nullptr;
			bool m_owns = false;
			bool m_initialized = false;
			static void switchToThisAsm(UINTPTR_T address);
		};
		// Only valid in x86_64.
		class PageMap
		{
		public:
			PageMap();
			// 'address' must be a physical address.
			PageMap(UINTPTR_T* address);

			// Use if you want to modify the entry in the level 4 page map.
			UINTPTR_T* getLevel3PageMapAddress(UINT16_T level3PageMap);
			// Use if you want to modify the entry in the level 3 page map. Returns an array.
			// Can also be called getLevel4PageMapEntry().	
			UINTPTR_T* getLevel3PageMap(UINT16_T level3PageMap);
			// Use if you want to modify the entry in the page directory. Returns an array.
			// Can also be called getLevel3PageMapEntry()
			UINTPTR_T* getPageDirectory(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex);
			UINTPTR_T* getPageDirectoryRealtive(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex);
			// Use if you want to modify the entry in the page table. Returns an array.
			// Can also be called getPageDirectoryEntry()
			UINTPTR_T* getPageTable(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex);
			UINTPTR_T* getPageTableRealtive(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex);
			// Returns a physical address.
			UINTPTR_T getPageTableEntry(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex, UINT16_T index);
			UINTPTR_T getRealPageTableEntry(UINT16_T level3PageMap, UINT16_T pageDirectoryIndex, UINT16_T pageTableIndex, UINT16_T index);
			
			UINTPTR_T* getPageMap() const { return m_array; }

			void switchToThis();

			~PageMap();

			static UINT16_T computeIndexAtAddress(UINTPTR_T address, UINT8_T level) { return (address >> (9 * level + 12)) & 0x1FF; }
		private:
			UINTPTR_T* m_array = nullptr;
			bool m_owns = false;
			bool m_initialized = false;
		};
		extern PageDirectory* g_pageDirectory;
		extern PageMap* g_level4PageMap;
		extern PageMap g_kernelPageMap;
		extern UINTPTR_T* g_zeroPage;
		void InitializePaging();

		void tlbFlush(UINTPTR_T addr);
		PVOID GetPageFaultAddress();
	}
}