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
		class PageDirectory final
		{
		public:
			PageDirectory();
			PageDirectory(UINTPTR_T* array);

			void init();

			void switchToThis();

			// Always check if the address of the return value of this function is not nullptr.
			// If there was a page table allocated, it will reallocate it.
			UINTPTR_T* getPageTableAddress(UINT16_T pageTable);
			// Always check if the address of the return value of this function is not nullptr.
			// If there was a page table allocated, it will reallocate it.
			UINTPTR_T* getPageTable(UINT16_T pageTable);

			UINTPTR_T* getPageDirectory() { return m_array; }

			~PageDirectory();

			static UINT16_T addressToIndex(UINTPTR_T base);
			static UINT16_T addressToPageTableIndex(UINTPTR_T base);
		private:
			// NOTE: Do not change the position of this variable. Don't to do it if you don't want to have a bad time.
			UINTPTR_T* m_array = nullptr;
			bool m_owns = false;
			bool m_initialized = false;
			static void switchToThisAsm(UINTPTR_T address);
		};
		extern PageDirectory* g_pageDirectory;
		// Makes a page directory and switches to it, thus enabling paging.
		void InitializePaging();

		void tlbFlush(UINT32_T addr);
		PVOID GetPageFaultAddress();
	}
}