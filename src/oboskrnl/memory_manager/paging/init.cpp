/*
	oboskrnl/memory_manager/paging/init.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/init.h>
#include <memory_manager/physical.h>

#include <utils/bitfields.h>

#define indexToAddress(index, pageTableIndex) ((UINTPTR_T)(((index) << 22) + ((pageTableIndex) << 12)))
#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && val <= ((UINTPTR_T)(rEnd)))

extern "C" char _ZN4obos6memory13PageDirectoryC1Ev;

namespace obos
{
	namespace memory
	{
		bool g_enabledPaging = false;

		PageDirectory::PageDirectory()
		{
			init();
		}
		PageDirectory::PageDirectory(UINTPTR_T* array)
		{
			m_array = array;
			m_initialized = true;
		}

		void PageDirectory::init()
		{
			if (m_initialized)
				destroy();
			m_array = (UINTPTR_T*)kalloc_physicalPages(1);
			m_owns = m_initialized = true;
		}

		void PageDirectory::switchToThis()
		{
			if (!m_initialized)
				return;
			switchToThisAsm((UINTPTR_T)m_array);
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
			if (m_initialized && m_owns)
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
		UINT16_T PageDirectory::addressToPageTableIndex(UINTPTR_T base)
		{
			return (base >> 12) % 1024;
		}

		extern void enablePaging();
		
		PageDirectory* g_pageDirectory;
		UINTPTR_T attribute(aligned(4096)) g_kernelPageDirectory[1024];

		void InitializePaging()
		{
			/*auto constructor = (void(*)(obos::memory::PageDirectory* _this))GET_FUNC_ADDR(&_ZN4obos6memory13PageDirectoryC1Ev);
			constructor(g_pageDirectory);*/
			if (g_enabledPaging)
				return;
			*g_pageDirectory->getPageTableAddress(0) = (UINTPTR_T)kalloc_physicalPages(1);
			*g_pageDirectory->getPageTableAddress(0) |= 3;
			for(UINTPTR_T i = 0, address = 0; i < 1024; i++, address += 4096)
			{
				if (i != 1023)
					*g_pageDirectory->getPageTableAddress(i + 1) = 2;
				utils::IntegerBitfield flags;
				flags.setBitfield(indexToAddress(0, i));
				flags.setBit(0);
				if (!inRange(address, 0x100000, (UINTPTR_T)&__erodata))
					flags.setBit(1);
				g_pageDirectory->getPageTable(0)[i] = flags.operator UINT32_T();
			}
			g_pageDirectory->switchToThis();
			enablePaging();
			g_enabledPaging = true;
		}
	}
}