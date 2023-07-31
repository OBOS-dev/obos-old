/*
	oboskrnl/memory_manager/paging/init.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/init.h>
#include <memory_manager/physical.h>

#include <utils/bitfields.h>

#define indexToAddress(index, pageTableIndex) ((UINTPTR_T)(((index) << 22) + ((pageTableIndex) << 12)))
#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && val <= ((UINTPTR_T)(rEnd)))

namespace obos
{
	namespace memory
	{
		PageDirectory::PageDirectory()
		{
			init();
		}

		void PageDirectory::init()
		{
			if (m_initialized)
				destroy();
			m_array = (UINTPTR_T*)kalloc_physicalPages(1);
			m_initialized = true;
		}

		UINTPTR_T* PageDirectory::getPageTableAddress(UINT16_T pageTable)
		{
			if (pageTable > 1023 || !m_initialized)
				return nullptr;
			return m_array + pageTable;
		}
		UINTPTR_T* PageDirectory::getPageTable(UINT16_T pageTable)
		{
			if (pageTable > 1023 || !m_initialized)
				return nullptr;
			return (UINTPTR_T*)(m_array[pageTable] & 0xFFFFF000);
		}

		void PageDirectory::destroy()
		{
			if (m_initialized)
			{
				// the next two lines are fine, look in kfree_physicalPages before changing them.
				for (int i = 0; i < 1024; i++)
					kfree_physicalPages((PVOID)m_array[i], 1);
				kfree_physicalPages(m_array, 1);
			}
		}

		PageDirectory::~PageDirectory()
		{
			destroy();
		}

		UINT16_T PageDirectory::addressToIndex(UINTPTR_T base)
		{
			return base >> 22;
		}

		PageDirectory g_pageDirectory;
		
		void InitializePaging()
		{
			// I should really stop doing this...
			extern char _ZN4obos6memory13PageDirectoryC1Ev;
			auto constructor = (void(*)(obos::memory::PageDirectory* _this))GET_FUNC_ADDR(_ZN4obos6memory13PageDirectoryC1Ev);
			constructor(&g_pageDirectory);
			*g_pageDirectory.getPageTableAddress(0) = (UINTPTR_T)kalloc_physicalPages(1);
			for(UINTPTR_T i = 0, address = 0; i < 1024; i++, address += 4096)
			{
				utils::IntegerBitfield flags = indexToAddress(0, i);
				flags.setBit(0);
				flags.setBit(1);
				if (inRange(address, 0x100000, (UINTPTR_T)&__erodata))
					flags.clearBit(1);
				g_pageDirectory.getPageTable(0)[i] = flags;
			}
			g_pageDirectory.switchToThis();
		}
	}
}