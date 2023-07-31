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

			void init();

			void switchToThis();

			// Always check if the address of the return value of this function is not nullptr.
			// 
			UINTPTR_T* getPageTableAddress(UINT16_T pageTable);
			// Always check if the address of the return value of this function is not nullptr.
			UINTPTR_T* getPageTable(UINT16_T pageTable);

			void destroy();

			~PageDirectory();

			static UINT16_T addressToIndex(UINTPTR_T base);
		private:
			UINTPTR_T* m_array = nullptr;
			bool m_initialized = false;
		};

		// Makes a page directory and switches to it, thus enabling paging.
		void InitializePaging();

		PVOID GetPageFaultAddress();
	}
}